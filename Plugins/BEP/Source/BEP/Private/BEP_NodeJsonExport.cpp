#include "BEP_NodeJsonExport.h"


#include "Algo/Sort.h"
#include "BEPExportSettings.h"
#include "BEP_NodeJsonExportSettings.h"
#include "BlueprintFlowExporter.h"

#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Toolkits/AssetEditorManager.h"
#include "Widgets/SGraphEditor.h"
#include "Widgets/SWidget.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogBEPNodeJson, Log, All);

namespace
{
constexpr int32 SchemaVersion = 1;
constexpr int32 CommentSchemaVersion = 1;
const FName KExecPinCategory(UEdGraphSchema_K2::PC_Exec);
const TCHAR* NodeJsonRootFolder = TEXT("BEP/NodeJSON");
const TCHAR* CommentTemplateSuffix = TEXT(".template.csv");

FString PresetToString(const EBEP_NodeJsonPreset Preset)
{
    if (const UEnum* Enum = StaticEnum<EBEP_NodeJsonPreset>())
    {
        return Enum->GetNameStringByValue(static_cast<int64>(Preset));
    }
    return TEXT("Unknown");
}

FString PinContainerToString(const EPinContainerType Container)
{
    if (const UEnum* Enum = StaticEnum<EPinContainerType>())
    {
        return Enum->GetNameStringByValue(static_cast<int64>(Container));
    }
    switch (Container)
    {
    case EPinContainerType::None: return TEXT("None");
    case EPinContainerType::Array: return TEXT("Array");
    case EPinContainerType::Set: return TEXT("Set");
    case EPinContainerType::Map: return TEXT("Map");
    default: return TEXT("Unknown");
    }
}

struct FNodeBfsState
{
    UEdGraphNode* Node = nullptr;
    int32 ExecDepth = 0;
    int32 DataDepth = 0;
};

FString SafeNodeId(UEdGraphNode* Node, int32 FallbackIndex)
{
    if (Node && Node->NodeGuid.IsValid())
    {
        return Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
    }
    return FString::Printf(TEXT("N%d"), FallbackIndex);
}

bool IsExecPin(const UEdGraphPin* Pin)
{
    return Pin && Pin->PinType.PinCategory == KExecPinCategory;
}

void GatherGraphEditors(const TSharedRef<SWidget>& Root, TArray<TSharedPtr<SGraphEditor>>& Out)
{
    if (Root->GetType() == FName(TEXT("SGraphEditor")))
    {
        Out.Add(StaticCastSharedRef<SGraphEditor>(Root));
    }
    FChildren* Children = Root->GetChildren();
    if (Children)
    {
        const int32 Num = Children->Num();
        for (int32 i = 0; i < Num; ++i)
        {
            TSharedRef<SWidget> Child = Children->GetChildAt(i);
            GatherGraphEditors(Child, Out);
        }
    }
}

TSharedPtr<SGraphEditor> FindActiveGraphEditor()
{
    // Prefer graph editors with a selection, else first available.
    const TArray<TSharedRef<SWindow>>& Windows = FSlateApplication::Get().GetTopLevelWindows();
    TArray<TSharedPtr<SGraphEditor>> Editors;
    for (const TSharedRef<SWindow>& Win : Windows)
    {
        if (TSharedPtr<SWidget> Content = Win->GetContent())
        {
            GatherGraphEditors(Content.ToSharedRef(), Editors);
        }
    }

    for (const TSharedPtr<SGraphEditor>& GE : Editors)
    {
        if (GE.IsValid())
        {
            if (UEdGraph* Graph = GE->GetCurrentGraph())
            {
                if (GE->GetSelectedNodes().Num() > 0)
                {
                    return GE;
                }
            }
        }
    }

    return Editors.Num() > 0 ? Editors[0] : nullptr;
}

UEdGraph* GetGraphFromEditor(const TSharedPtr<SGraphEditor>& GraphEditor)
{
    return GraphEditor.IsValid() ? GraphEditor->GetCurrentGraph() : nullptr;
}

UBlueprint* GetBlueprintForGraph(UEdGraph* Graph)
{
    if (!Graph)
    {
        return nullptr;
    }

    if (UBlueprint* Direct = Graph->GetTypedOuter<UBlueprint>())
    {
        return Direct;
    }

    return FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
}

FString SanitizeFileStem(const FString& In)
{
    FString Stem = In;
    Stem.ReplaceInline(TEXT(" "), TEXT("_"));
    Stem = FPaths::MakeValidFileName(Stem);
    if (Stem.IsEmpty())
    {
        Stem = TEXT("Export");
    }
    return Stem;
}

FString BlueprintSegmentFromStem(const FString& Stem)
{
    TArray<FString> Parts;
    Stem.ParseIntoArray(Parts, TEXT("__"), true);
    return Parts.Num() > 0 ? FPaths::MakeValidFileName(Parts[0]) : SanitizeFileStem(Stem);
}

FString MakeFilenameSafe(const FString& In)
{
    FString Out = In;
    static const TCHAR* BadChars[] = { TEXT("\\"), TEXT("/"), TEXT(":"), TEXT("*"), TEXT("?"), TEXT("\""), TEXT("<"), TEXT(">"), TEXT("|") };
    for (const TCHAR* Bad : BadChars)
    {
        Out.ReplaceInline(Bad, TEXT("_"));
    }
    Out = FPaths::MakeValidFileName(Out);

    // Collapse whitespace runs to underscore
    FString Collapsed;
    Collapsed.Reserve(Out.Len());
    bool bInSpace = false;
    for (const TCHAR Ch : Out)
    {
        if (FChar::IsWhitespace(Ch))
        {
            if (!bInSpace)
            {
                Collapsed.AppendChar(TEXT('_'));
                bInSpace = true;
            }
        }
        else
        {
            Collapsed.AppendChar(Ch);
            bInSpace = false;
        }
    }

    if (Collapsed.Len() > 120)
    {
        Collapsed = Collapsed.Left(120);
    }

    return Collapsed.TrimStartAndEnd();
}

FString GetBEPExportRoot()
{
    FString Root = GetDefaultBEPExportRoot();
    if (Root.IsEmpty())
    {
        Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("BEP_EXPORTS"));
    }
    return Root;
}

void SplitStemForFolders(const FString& Stem, FString& OutBlueprint, FString& OutGraph)
{
    TArray<FString> Parts;
    Stem.ParseIntoArray(Parts, TEXT("__"), true);
    OutBlueprint = Parts.IsValidIndex(0) ? Parts[0] : TEXT("Blueprint");
    OutGraph = Parts.IsValidIndex(1) ? Parts[1] : TEXT("Graph");
    OutBlueprint = MakeFilenameSafe(OutBlueprint);
    OutGraph = MakeFilenameSafe(OutGraph);
}

FString BuildNodeJsonDir(const FString& Stem, bool bGoldenSample)
{
    FString BlueprintFolder;
    FString GraphFolder;
    SplitStemForFolders(Stem, BlueprintFolder, GraphFolder);

    FString Root = GetBEPExportRoot();
    FString Base = FPaths::Combine(Root, NodeJsonRootFolder);

    if (bGoldenSample)
    {
        return FPaths::Combine(Base, TEXT("GoldenSamples"));
    }

    return FPaths::Combine(Base, BlueprintFolder, GraphFolder);
}

FString BuildFilePath(const FString& Stem, const FString& Extension, bool bGoldenSample)
{
    const FString Dir = BuildNodeJsonDir(Stem, bGoldenSample);
    IFileManager::Get().MakeDirectory(*Dir, true);
    return FPaths::Combine(Dir, Stem + Extension);
}

void WriteSummary(const FString& SummaryPath, const FString& JsonPath, const FBEP_NodeJsonExportOptions& Opt, const FBEP_NodeJsonExportStats& Stats)
{
    FString Summary;
    Summary += FString::Printf(TEXT("Timestamp (UTC): %s\n"), *FDateTime::UtcNow().ToIso8601());
    Summary += FString::Printf(TEXT("Preset: %s\n"), *PresetToString(Opt.Preset));
    Summary += FString::Printf(TEXT("CompactKeys: %s, Pretty: %s, ClassDict: %s, Legend: %s\n"),
        Opt.bCompactKeys ? TEXT("true") : TEXT("false"),
        Opt.bPrettyPrint ? TEXT("true") : TEXT("false"),
        Opt.bIncludeClassDict ? TEXT("true") : TEXT("false"),
        Opt.bIncludeLegend ? TEXT("true") : TEXT("false"));
    Summary += FString::Printf(TEXT("ExecDepth: %d, DataDepth: %d\n"), Stats.ExecDepthUsed, Stats.DataDepthUsed);
    Summary += FString::Printf(TEXT("SeedSelection: %d\n"), Stats.SeedSelectionCount);
    Summary += FString::Printf(TEXT("ExpandedNodes: %d\n"), Stats.ExpandedNodeCount);
    Summary += FString::Printf(TEXT("FinalNodes: %d (NodeCap=%d, Hit=%s)\n"), Stats.FinalNodeCount, Stats.MaxNodeCap, Stats.bHitNodeCap ? TEXT("true") : TEXT("false"));
    Summary += FString::Printf(TEXT("ExecEdges: %d, DataEdges: %d, TotalEdges: %d (EdgeCap=%d, Hit=%s)\n"),
        Stats.ExecEdgeCount, Stats.DataEdgeCount, Stats.ExecEdgeCount + Stats.DataEdgeCount, Stats.MaxEdgeCap, Stats.bHitEdgeCap ? TEXT("true") : TEXT("false"));
    Summary += FString::Printf(TEXT("EngineTag: %s\n"), *Opt.EngineTag);
    Summary += FString::Printf(TEXT("JSON: %s\n"), *JsonPath);

    FFileHelper::SaveStringToFile(Summary, *SummaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

TSharedRef<FJsonObject> MakePinTypeJson(const FEdGraphPinType& PinType, bool bCompact)
{
    TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
    const FString Cat = PinType.PinCategory.ToString();
    const FString SubCat = PinType.PinSubCategory.ToString();
    const FString ObjPath = PinType.PinSubCategoryObject.IsValid()
        ? PinType.PinSubCategoryObject.ToSoftObjectPath().ToString()
        : FString();
    const FString Container = PinContainerToString(PinType.ContainerType);

    if (bCompact)
    {
        Obj->SetStringField(TEXT("cg"), Cat);
        if (!PinType.PinSubCategory.IsNone())
        {
            Obj->SetStringField(TEXT("sc"), SubCat);
        }
        if (!ObjPath.IsEmpty())
        {
            Obj->SetStringField(TEXT("sco"), ObjPath);
        }
        if (PinType.ContainerType != EPinContainerType::None)
        {
            Obj->SetStringField(TEXT("ct"), Container);
        }
        if (PinType.bIsReference)
        {
            Obj->SetBoolField(TEXT("rf"), true);
        }
        if (PinType.bIsConst)
        {
            Obj->SetBoolField(TEXT("cs"), true);
        }
    }
    else
    {
        Obj->SetStringField(TEXT("category"), Cat);
        if (!PinType.PinSubCategory.IsNone())
        {
            Obj->SetStringField(TEXT("sub_category"), SubCat);
        }
        if (!ObjPath.IsEmpty())
        {
            Obj->SetStringField(TEXT("sub_category_object"), ObjPath);
        }
        if (PinType.ContainerType != EPinContainerType::None)
        {
            Obj->SetStringField(TEXT("container_type"), Container);
        }
        if (PinType.bIsReference)
        {
            Obj->SetBoolField(TEXT("is_reference"), true);
        }
        if (PinType.bIsConst)
        {
            Obj->SetBoolField(TEXT("is_const"), true);
        }
    }
    return Obj;
}

void AddPinFlags(const UEdGraphPin* Pin, const TSharedRef<FJsonObject>& PinObj, bool bCompact)
{
    if (!Pin)
    {
        return;
    }

    TSharedRef<FJsonObject> Flags = MakeShared<FJsonObject>();
    Flags->SetBoolField(bCompact ? TEXT("h") : TEXT("hidden"), Pin->bHidden);
    Flags->SetBoolField(bCompact ? TEXT("nc") : TEXT("not_connectable"), Pin->bNotConnectable);
    Flags->SetBoolField(bCompact ? TEXT("dvro") : TEXT("default_readonly"), Pin->bDefaultValueIsReadOnly);
    Flags->SetBoolField(bCompact ? TEXT("dvi") : TEXT("default_ignored"), Pin->bDefaultValueIsIgnored);
    Flags->SetBoolField(bCompact ? TEXT("dif") : TEXT("diffing"), Pin->bIsDiffing);

    const bool bAny = Pin->bHidden || Pin->bNotConnectable || Pin->bDefaultValueIsReadOnly || Pin->bDefaultValueIsIgnored || Pin->bIsDiffing;
    if (bAny)
    {
        PinObj->SetObjectField(bCompact ? TEXT("fl") : TEXT("flags"), Flags);
    }
}

} // namespace

void BEP_NodeJson::ApplyPreset(FBEP_NodeJsonExportOptions& Opt, EBEP_NodeJsonPreset Preset)
{
    Opt.Preset = Preset;
    Opt.MaxNodesHardCap = 500;
    Opt.MaxEdgesHardCap = 2000;
    Opt.bWarnOnCapHit = true;

    switch (Preset)
    {
    case EBEP_NodeJsonPreset::SuperCompact:
        Opt.bCompactKeys = true;
        Opt.bPrettyPrint = false;
        Opt.bIncludeClassDict = true;
        Opt.bIncludeLegend = false;
        Opt.bIncludePosition = true;
        Opt.bIncludePinDecls = true;
        Opt.bIncludeFullPins = false;
        Opt.bIncludeUnconnectedPins = false;
        Opt.bIncludeNodeTitle = false;
        Opt.bIncludeNodeComment = false;
        Opt.bIncludeExecEdges = true;
        Opt.bIncludeDataEdges = true;
        Opt.ExecDepth = 0;
        Opt.DataDepth = 0;
        break;
    case EBEP_NodeJsonPreset::Compact:
        Opt.bCompactKeys = true;
        Opt.bPrettyPrint = false;
        Opt.bIncludeClassDict = true;
        Opt.bIncludeLegend = false;
        Opt.bIncludePosition = true;
        Opt.bIncludePinDecls = true;
        Opt.bIncludeFullPins = false;
        Opt.bIncludeUnconnectedPins = false;
        Opt.bIncludeNodeTitle = true;
        Opt.bIncludeNodeComment = false;
        Opt.bIncludeExecEdges = true;
        Opt.bIncludeDataEdges = true;
        Opt.ExecDepth = 0;
        Opt.DataDepth = 0;
        break;
    case EBEP_NodeJsonPreset::AIMinimal:
        Opt.bCompactKeys = false;
        Opt.bPrettyPrint = true;
        Opt.bIncludeClassDict = false;
        Opt.bIncludeLegend = false;
        Opt.bIncludePosition = true;
        Opt.bIncludePinDecls = true;
        Opt.bIncludeFullPins = false;
        Opt.bIncludeUnconnectedPins = false;
        Opt.bIncludeNodeTitle = true;
        Opt.bIncludeNodeComment = false;
        Opt.bIncludeExecEdges = true;
        Opt.bIncludeDataEdges = true;
        Opt.ExecDepth = 0;
        Opt.DataDepth = 0;
        break;
    case EBEP_NodeJsonPreset::AIAudit:
        Opt.bCompactKeys = false;
        Opt.bPrettyPrint = true;
        Opt.bIncludeClassDict = false;
        Opt.bIncludeLegend = false;
        Opt.bIncludePosition = true;
        Opt.bIncludePinDecls = true;
        Opt.bIncludeFullPins = true;
        Opt.bIncludeUnconnectedPins = true;
        Opt.bIncludeNodeTitle = true;
        Opt.bIncludeNodeComment = true;
        Opt.bIncludeExecEdges = true;
        Opt.bIncludeDataEdges = true;
        Opt.ExecDepth = 0;
        Opt.DataDepth = 0;
        break;
    case EBEP_NodeJsonPreset::Custom:
    default:
        break;
    }
}

bool BEP_NodeJson::ExportActiveSelectionToJson(const FBEP_NodeJsonExportOptions& Opt, FString& OutJson, FString& OutSuggestedFileStem, int32& OutNodeCount, int32& OutEdgeCount, FString* OutError, FBEP_NodeJsonExportStats* OutStats, bool bPreviewOnly)
{
    OutJson.Reset();
    OutSuggestedFileStem.Reset();
    OutNodeCount = 0;
    OutEdgeCount = 0;

    FBEP_NodeJsonExportStats LocalStats;
    FBEP_NodeJsonExportStats* Stats = OutStats ? OutStats : &LocalStats;
    Stats->MaxNodeCap = Opt.MaxNodesHardCap;
    Stats->MaxEdgeCap = Opt.MaxEdgesHardCap;
    Stats->ExecDepthUsed = Opt.ExecDepth;
    Stats->DataDepthUsed = Opt.DataDepth;

    const TSharedPtr<SGraphEditor> GraphEditor = FindActiveGraphEditor();
    UEdGraph* Graph = GetGraphFromEditor(GraphEditor);
    if (!Graph)
    {
        if (OutError) { *OutError = TEXT("No active graph editor."); }
        return false;
    }

    const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
    Stats->SeedSelectionCount = Selection.Num();
    if (Selection.Num() == 0)
    {
        if (OutError) { *OutError = TEXT("No nodes selected."); }
        return false;
    }

    // BFS expansion
    TSet<UEdGraphNode*> Visited;
    TQueue<FNodeBfsState> Queue;

    for (UObject* Obj : Selection)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            Visited.Add(Node);
            Queue.Enqueue({Node, 0, 0});
        }
    }

    const int32 MaxExec = FMath::Max(0, Opt.ExecDepth);
    const int32 MaxData = FMath::Max(0, Opt.DataDepth);

    while (!Queue.IsEmpty())
    {
        FNodeBfsState State;
        Queue.Dequeue(State);

        UEdGraphNode* Node = State.Node;
        if (!Node)
        {
            continue;
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin)
            {
                continue;
            }

            const bool bIsExec = IsExecPin(Pin);
            const int32 NextExec = State.ExecDepth + (bIsExec ? 1 : 0);
            const int32 NextData = State.DataDepth + (bIsExec ? 0 : 1);

            if ((bIsExec && NextExec > MaxExec) || (!bIsExec && NextData > MaxData))
            {
                continue;
            }

            for (UEdGraphPin* Linked : Pin->LinkedTo)
            {
                if (!Linked)
                {
                    continue;
                }

                UEdGraphNode* Neighbor = Linked->GetOwningNode();
                if (!Neighbor || Neighbor->GetGraph() != Graph)
                {
                    continue;
                }

                if (!Visited.Contains(Neighbor))
                {
                    Visited.Add(Neighbor);
                    Queue.Enqueue({Neighbor, NextExec, NextData});
                }
            }
        }
    }

    Stats->ExpandedNodeCount = Visited.Num();

    if (Visited.Num() == 0)
    {
        if (OutError) { *OutError = TEXT("No nodes after expansion."); }
        return false;
    }

    // Stable IDs and sorted traversal
    TMap<UEdGraphNode*, FString> NodeIds;
    int32 Index = 0;
    for (UEdGraphNode* Node : Visited)
    {
        NodeIds.Add(Node, SafeNodeId(Node, ++Index));
    }

    TArray<UEdGraphNode*> SortedNodes = Visited.Array();
    SortedNodes.Sort([&](UEdGraphNode& A, UEdGraphNode& B)
    {
        const FString* AId = NodeIds.Find(&A);
        const FString* BId = NodeIds.Find(&B);
        return (AId ? *AId : FString()) < (BId ? *BId : FString());
    });

    UBlueprint* Blueprint = GetBlueprintForGraph(Graph);
    const FString BlueprintName = MakeFilenameSafe(Blueprint ? Blueprint->GetName() : Graph->GetName());
    const FString GraphName = MakeFilenameSafe(Graph->GetName());
    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    OutSuggestedFileStem = FString::Printf(TEXT("%s__%s__SelectedNodes_%s"), *BlueprintName, *GraphName, *Timestamp);

    const bool bCompact = Opt.bCompactKeys;

    auto Key = [bCompact](const TCHAR* Long, const TCHAR* Short)
    {
        return bCompact ? Short : Long;
    };

    TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
    RootObj->SetNumberField(Key(TEXT("schema_version"), TEXT("sv")), SchemaVersion);
    RootObj->SetStringField(Key(TEXT("engine"), TEXT("e")), Opt.EngineTag);

    if (Opt.bIncludeAssetPath && Blueprint)
    {
        RootObj->SetStringField(Key(TEXT("asset"), TEXT("a")), Blueprint->GetPathName());
    }
    if (Opt.bIncludeGraphName)
    {
        RootObj->SetStringField(Key(TEXT("graph"), TEXT("g")), GraphName);
    }

    // Class dict for compact codes
    TMap<FString, FString> ClassToCode;
    TMap<FString, FString> CodeToClass;
    int32 ClassCounter = 0;

    auto GetClassField = [&](const FString& ClassName)
    {
        if (Opt.bIncludeClassDict && bCompact)
        {
            if (const FString* Existing = ClassToCode.Find(ClassName))
            {
                return *Existing;
            }

            const FString Code = FString::Printf(TEXT("c%d"), ClassCounter++);
            ClassToCode.Add(ClassName, Code);
            CodeToClass.Add(Code, ClassName);
            return Code;
        }

        return ClassName;
    };

    // Node cap handling
    if (Opt.MaxNodesHardCap > 0 && SortedNodes.Num() > Opt.MaxNodesHardCap)
    {
        Stats->bHitNodeCap = true;
        if (Opt.bWarnOnCapHit)
        {
            UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Cap hit: nodes requested %d, clamped to %d"), SortedNodes.Num(), Opt.MaxNodesHardCap);
        }
        SortedNodes.SetNum(Opt.MaxNodesHardCap);
    }

    // Nodes
    TArray<TSharedPtr<FJsonValue>> NodeValues;
    NodeValues.Reserve(SortedNodes.Num());

    auto BuildPinDecl = [&](UEdGraphPin* Pin) -> TSharedRef<FJsonObject>
    {
        TSharedRef<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(Key(TEXT("name"), TEXT("n")), Pin->PinName.ToString());
        PinObj->SetNumberField(Key(TEXT("dir"), TEXT("d")), Pin->Direction == EGPD_Input ? 0 : 1);
        PinObj->SetNumberField(Key(TEXT("kind"), TEXT("k")), IsExecPin(Pin) ? 0 : 1);
        PinObj->SetObjectField(Key(TEXT("type"), TEXT("t")), MakePinTypeJson(Pin->PinType, bCompact));

        if (Opt.bIncludeDefaultValues)
        {
            const FString DefaultVal = FBlueprintFlowExporter::GetPinDefaultValueString(Pin);
            if (!DefaultVal.IsEmpty())
            {
                PinObj->SetStringField(Key(TEXT("default_raw"), TEXT("dr")), DefaultVal);
            }
        }

        if (Opt.bIncludePinFlags)
        {
            AddPinFlags(Pin, PinObj, bCompact);
        }

        return PinObj;
    };

    for (UEdGraphNode* Node : SortedNodes)
    {
        if (!Node)
        {
            continue;
        }

        TSharedRef<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        const FString NodeId = NodeIds.FindChecked(Node);
        NodeObj->SetStringField(Key(TEXT("id"), TEXT("i")), NodeId);

        const FString ClassName = Node->GetClass() ? Node->GetClass()->GetPathName() : FString(TEXT("Unknown"));
        NodeObj->SetStringField(Key(TEXT("class"), TEXT("c")), GetClassField(ClassName));

        if (Opt.bIncludeNodeTitle)
        {
            NodeObj->SetStringField(Key(TEXT("name"), TEXT("nm")), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
        }
        if (Opt.bIncludeNodeComment && !Node->NodeComment.IsEmpty())
        {
            NodeObj->SetStringField(Key(TEXT("comment"), TEXT("cm")), Node->NodeComment);
        }

        if (Opt.bIncludePosition)
        {
            TSharedRef<FJsonObject> Pos = MakeShared<FJsonObject>();
            Pos->SetNumberField(Key(TEXT("x"), TEXT("x")), Node->NodePosX);
            Pos->SetNumberField(Key(TEXT("y"), TEXT("y")), Node->NodePosY);
            NodeObj->SetObjectField(Key(TEXT("pos"), TEXT("p")), Pos);
        }

        if (Opt.bIncludePinDecls)
        {
            TArray<TSharedPtr<FJsonValue>> PinDecls;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin)
                {
                    continue;
                }
                PinDecls.Add(MakeShared<FJsonValueObject>(BuildPinDecl(Pin)));
            }

            PinDecls.Sort([&](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
            {
                const TSharedPtr<FJsonObject>* AO;
                const TSharedPtr<FJsonObject>* BO;
                if (!A->TryGetObject(AO) || !B->TryGetObject(BO))
                {
                    return false;
                }
                const FString AN = (*AO)->GetStringField(Key(TEXT("name"), TEXT("n")));
                const FString BN = (*BO)->GetStringField(Key(TEXT("name"), TEXT("n")));
                const int32 AD = static_cast<int32>((*AO)->GetNumberField(Key(TEXT("dir"), TEXT("d"))));
                const int32 BD = static_cast<int32>((*BO)->GetNumberField(Key(TEXT("dir"), TEXT("d"))));
                const int32 AK = static_cast<int32>((*AO)->GetNumberField(Key(TEXT("kind"), TEXT("k"))));
                const int32 BK = static_cast<int32>((*BO)->GetNumberField(Key(TEXT("kind"), TEXT("k"))));
                if (AD != BD) return AD < BD;
                if (AK != BK) return AK < BK;
                return AN < BN;
            });

            if (PinDecls.Num() > 0)
            {
                NodeObj->SetArrayField(Key(TEXT("pin_decls"), TEXT("pd")), PinDecls);
            }
        }

        if (Opt.bIncludeFullPins || Opt.bIncludeUnconnectedPins)
        {
            TArray<TSharedPtr<FJsonValue>> FullPins;
            TArray<TSharedPtr<FJsonValue>> UnconnectedPins;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin)
                {
                    continue;
                }

                const bool bUnconnected = Pin->LinkedTo.Num() == 0;

                if (Opt.bIncludeFullPins)
                {
                    FullPins.Add(MakeShared<FJsonValueObject>(BuildPinDecl(Pin)));
                }

                if (Opt.bIncludeUnconnectedPins && bUnconnected)
                {
                    UnconnectedPins.Add(MakeShared<FJsonValueObject>(BuildPinDecl(Pin)));
                }
            }

            if (Opt.bIncludeFullPins && FullPins.Num() > 0)
            {
                NodeObj->SetArrayField(Key(TEXT("pins"), TEXT("p")), FullPins);
            }
            if (Opt.bIncludeUnconnectedPins && UnconnectedPins.Num() > 0)
            {
                NodeObj->SetArrayField(Key(TEXT("pins_unconnected"), TEXT("pu")), UnconnectedPins);
            }
        }

        NodeValues.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    RootObj->SetArrayField(Key(TEXT("nodes"), TEXT("n")), NodeValues);
    OutNodeCount = NodeValues.Num();
    Stats->FinalNodeCount = OutNodeCount;

    // Edges
    TArray<TSharedPtr<FJsonValue>> EdgeValues;
    int32 ExecEdgeCount = 0;
    int32 DataEdgeCount = 0;
    if (Opt.bIncludeExecEdges || Opt.bIncludeDataEdges)
    {
        TSet<FString> EdgeKeys;

        auto ShouldIncludeEdge = [&](bool bIsExec)
        {
            return (bIsExec && Opt.bIncludeExecEdges) || (!bIsExec && Opt.bIncludeDataEdges);
        };

        for (UEdGraphNode* Node : SortedNodes)
        {
            if (!Node)
            {
                continue;
            }

            const FString FromId = NodeIds.FindChecked(Node);

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin)
                {
                    continue;
                }

                const bool bExec = IsExecPin(Pin);
                if (!ShouldIncludeEdge(bExec))
                {
                    continue;
                }

                const FString FromPinName = Pin->PinName.ToString();

                for (UEdGraphPin* Linked : Pin->LinkedTo)
                {
                    if (!Linked)
                    {
                        continue;
                    }

                    UEdGraphNode* OtherNode = Linked->GetOwningNode();
                    if (!OtherNode || !Visited.Contains(OtherNode))
                    {
                        continue;
                    }

                    const FString ToId = NodeIds.FindChecked(OtherNode);
                    const FString ToPinName = Linked->PinName.ToString();
                    const FString EdgeKey = FString::Printf(TEXT("%d|%s|%s|%s|%s"), bExec ? 0 : 1, *FromId, *FromPinName, *ToId, *ToPinName);

                    if (EdgeKeys.Contains(EdgeKey))
                    {
                        continue;
                    }

                    EdgeKeys.Add(EdgeKey);

                    TSharedRef<FJsonObject> EdgeObj = MakeShared<FJsonObject>();
                    if (bCompact)
                    {
                        EdgeObj->SetNumberField(TEXT("k"), bExec ? 0 : 1);
                        EdgeObj->SetStringField(TEXT("fn"), FromId);
                        EdgeObj->SetStringField(TEXT("fp"), FromPinName);
                        EdgeObj->SetStringField(TEXT("tn"), ToId);
                        EdgeObj->SetStringField(TEXT("tp"), ToPinName);
                    }
                    else
                    {
                        EdgeObj->SetStringField(TEXT("kind"), bExec ? TEXT("exec") : TEXT("data"));
                        EdgeObj->SetStringField(TEXT("from_node"), FromId);
                        EdgeObj->SetStringField(TEXT("from_pin"), FromPinName);
                        EdgeObj->SetStringField(TEXT("to_node"), ToId);
                        EdgeObj->SetStringField(TEXT("to_pin"), ToPinName);
                    }

                    EdgeValues.Add(MakeShared<FJsonValueObject>(EdgeObj));
                    if (bExec)
                    {
                        ++ExecEdgeCount;
                    }
                    else
                    {
                        ++DataEdgeCount;
                    }
                }
            }
        }

        EdgeValues.Sort([&](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
        {
            const TSharedPtr<FJsonObject>* AO;
            const TSharedPtr<FJsonObject>* BO;
            if (!A->TryGetObject(AO) || !B->TryGetObject(BO))
            {
                return false;
            }

            auto GetKind = [&](const TSharedPtr<FJsonObject>& Obj)
            {
                if (bCompact)
                {
                    double KindNum = 0.0;
                    Obj->TryGetNumberField(TEXT("k"), KindNum);
                    return static_cast<int32>(KindNum);
                }
                const FString KindStr = Obj->GetStringField(TEXT("kind"));
                return KindStr.Equals(TEXT("exec"), ESearchCase::IgnoreCase) ? 0 : 1;
            };

            const int32 AK = GetKind(*AO);
            const int32 BK = GetKind(*BO);

            auto F = [&](const TSharedPtr<FJsonObject>& Obj, const TCHAR* CompactKey, const TCHAR* VerboseKey)
            {
                return Obj->GetStringField(bCompact ? CompactKey : VerboseKey);
            };

            const FString AFN = F(*AO, TEXT("fn"), TEXT("from_node"));
            const FString BFN = F(*BO, TEXT("fn"), TEXT("from_node"));
            if (AK != BK) return AK < BK;
            if (AFN != BFN) return AFN < BFN;

            const FString AFP = F(*AO, TEXT("fp"), TEXT("from_pin"));
            const FString BFP = F(*BO, TEXT("fp"), TEXT("from_pin"));
            if (AFP != BFP) return AFP < BFP;

            const FString ATN = F(*AO, TEXT("tn"), TEXT("to_node"));
            const FString BTN = F(*BO, TEXT("tn"), TEXT("to_node"));
            if (ATN != BTN) return ATN < BTN;

            const FString ATP = F(*AO, TEXT("tp"), TEXT("to_pin"));
            const FString BTP = F(*BO, TEXT("tp"), TEXT("to_pin"));
            return ATP < BTP;
        });
    }

    if (Opt.MaxEdgesHardCap > 0 && EdgeValues.Num() > Opt.MaxEdgesHardCap)
    {
        Stats->bHitEdgeCap = true;
        if (Opt.bWarnOnCapHit)
        {
            UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Cap hit: edges requested %d, clamped to %d"), EdgeValues.Num(), Opt.MaxEdgesHardCap);
        }
        if (EdgeValues.Num() > 0)
        {
            EdgeValues.SetNum(Opt.MaxEdgesHardCap);
            // Recompute exec/data counts after clamp by iterating truncated edges
            ExecEdgeCount = 0;
            DataEdgeCount = 0;
            for (const TSharedPtr<FJsonValue>& Val : EdgeValues)
            {
                const TSharedPtr<FJsonObject>* ObjPtr;
                if (Val->TryGetObject(ObjPtr))
                {
                    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
                    if (bCompact)
                    {
                        double KindNum = 0.0;
                        Obj->TryGetNumberField(TEXT("k"), KindNum);
                        if (KindNum == 0.0)
                        {
                            ++ExecEdgeCount;
                        }
                        else
                        {
                            ++DataEdgeCount;
                        }
                    }
                    else
                    {
                        const FString KindStr = Obj->GetStringField(TEXT("kind"));
                        if (KindStr.Equals(TEXT("exec"), ESearchCase::IgnoreCase))
                        {
                            ++ExecEdgeCount;
                        }
                        else
                        {
                            ++DataEdgeCount;
                        }
                    }
                }
            }
        }
    }

    Stats->ExecEdgeCount = ExecEdgeCount;
    Stats->DataEdgeCount = DataEdgeCount;

    if (EdgeValues.Num() > 0)
    {
        RootObj->SetArrayField(Key(TEXT("edges"), TEXT("x")), EdgeValues);
    }
    OutEdgeCount = EdgeValues.Num();
    Stats->FinalNodeCount = OutNodeCount;

    if (bPreviewOnly)
    {
        return true;
    }

    if (bCompact && Opt.bIncludeClassDict && CodeToClass.Num() > 0)
    {
        TSharedRef<FJsonObject> DictObj = MakeShared<FJsonObject>();
        for (const TPair<FString, FString>& Pair : CodeToClass)
        {
            DictObj->SetStringField(Pair.Key, Pair.Value);
        }
        RootObj->SetObjectField(Key(TEXT("class_dict"), TEXT("cd")), DictObj);
    }

    if (bCompact && Opt.bIncludeLegend)
    {
        TSharedRef<FJsonObject> LegendObj = MakeShared<FJsonObject>();
        LegendObj->SetStringField(TEXT("sv"), TEXT("schema_version"));
        LegendObj->SetStringField(TEXT("e"), TEXT("engine"));
        LegendObj->SetStringField(TEXT("a"), TEXT("asset"));
        LegendObj->SetStringField(TEXT("g"), TEXT("graph"));
        LegendObj->SetStringField(TEXT("n"), TEXT("nodes"));
        LegendObj->SetStringField(TEXT("x"), TEXT("edges"));
        LegendObj->SetStringField(TEXT("pd"), TEXT("pin_decls"));
        LegendObj->SetStringField(TEXT("p"), TEXT("pins"));
        LegendObj->SetStringField(TEXT("pu"), TEXT("pins_unconnected"));
        RootObj->SetObjectField(TEXT("lg"), LegendObj);
    }

    if (Opt.bPrettyPrint)
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
        FJsonSerializer::Serialize(RootObj, Writer);
    }
    else
    {
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutJson);
        FJsonSerializer::Serialize(RootObj, Writer);
    }

    return !OutJson.IsEmpty();
}

bool BEP_NodeJson::SaveJsonToFile(const FString& Json, const FString& SuggestedFileStem, const FBEP_NodeJsonExportOptions& Opt, const FBEP_NodeJsonExportStats* Stats, FString& OutAbsPath, FString* OutSummaryPath, FString* OutError, bool bGoldenSample)
{
    OutAbsPath.Reset();
    if (OutSummaryPath)
    {
        OutSummaryPath->Reset();
    }

    const FString FilePath = BuildFilePath(SuggestedFileStem, TEXT(".json"), bGoldenSample);
    const bool bSaved = FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    if (!bSaved)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("Failed to save file: %s"), *FilePath);
        }
        return false;
    }

    OutAbsPath = FPaths::ConvertRelativePathToFull(FilePath);

    if (Stats && OutSummaryPath)
    {
        const FString SummaryStem = SuggestedFileStem + TEXT(".summary");
        const FString SummaryPath = BuildFilePath(SummaryStem, TEXT(".txt"), bGoldenSample);
        WriteSummary(SummaryPath, OutAbsPath, Opt, *Stats);
        *OutSummaryPath = FPaths::ConvertRelativePathToFull(SummaryPath);
    }

    return true;
}

bool BEP_NodeJson::ExportActiveSelectionToCommentJson(const FBEP_NodeJsonExportOptions& Opt, FString& OutJson, FString& OutSuggestedFileStem, TArray<FString>* OutGuids, FString* OutError)
{
    OutJson.Reset();
    OutSuggestedFileStem.Reset();

    const TSharedPtr<SGraphEditor> GraphEditor = FindActiveGraphEditor();
    UEdGraph* Graph = GetGraphFromEditor(GraphEditor);
    if (!Graph)
    {
        if (OutError) { *OutError = TEXT("No active graph editor."); }
        return false;
    }

    const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
    if (Selection.Num() == 0)
    {
        if (OutError) { *OutError = TEXT("No nodes selected."); }
        return false;
    }

    UBlueprint* Blueprint = GetBlueprintForGraph(Graph);
    const FString BlueprintName = MakeFilenameSafe(Blueprint ? Blueprint->GetName() : Graph->GetName());
    const FString GraphName = MakeFilenameSafe(Graph->GetName());
    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    OutSuggestedFileStem = FString::Printf(TEXT("%s__%s__Comments_%s"), *BlueprintName, *GraphName, *Timestamp);

    struct FCommentRow
    {
        FString Id;
        int32 X = 0;
        int32 Y = 0;
        FString Title;
        FString ClassPath;
        FString ExistingComment;
    };

    TArray<FCommentRow> Rows;
    Rows.Reserve(Selection.Num());
    if (OutGuids)
    {
        OutGuids->Reset();
        OutGuids->Reserve(Selection.Num());
    }

    for (UObject* Obj : Selection)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            if (!Node->NodeGuid.IsValid())
            {
                UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON][Comments] Skipping node with invalid Guid: %s"), *Node->GetName());
                continue;
            }

            FCommentRow Row;
            Row.Id = Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
            Row.X = Node->NodePosX;
            Row.Y = Node->NodePosY;

            if (Opt.bIncludeNodeTitle)
            {
                Row.Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            }
            if (Opt.bIncludeNodeComment && !Node->NodeComment.IsEmpty())
            {
                Row.ExistingComment = Node->NodeComment;
            }
            if (Opt.bIncludeClassDict && Node->GetClass())
            {
                Row.ClassPath = Node->GetClass()->GetPathName();
            }

            Rows.Add(MoveTemp(Row));
            if (OutGuids)
            {
                OutGuids->Add(Row.Id);
            }
        }
    }

    if (Rows.Num() == 0)
    {
        if (OutError) { *OutError = TEXT("No valid nodes with GUIDs selected."); }
        return false;
    }

    Rows.Sort([](const FCommentRow& A, const FCommentRow& B)
    {
        return A.Id < B.Id;
    });

    TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
    RootObj->SetNumberField(TEXT("schema_version"), CommentSchemaVersion);
    RootObj->SetStringField(TEXT("engine"), Opt.EngineTag);
    RootObj->SetStringField(TEXT("exported_utc"), FDateTime::UtcNow().ToIso8601());

    if (Opt.bIncludeAssetPath && Blueprint)
    {
        RootObj->SetStringField(TEXT("asset"), Blueprint->GetPathName());
    }
    if (Opt.bIncludeGraphName)
    {
        RootObj->SetStringField(TEXT("graph"), GraphName);
    }

    TArray<TSharedPtr<FJsonValue>> NodeValues;
    NodeValues.Reserve(Rows.Num());
    for (const FCommentRow& Row : Rows)
    {
        TSharedRef<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("id"), Row.Id);
        NodeObj->SetNumberField(TEXT("x"), Row.X);
        NodeObj->SetNumberField(TEXT("y"), Row.Y);

        if (!Row.Title.IsEmpty())
        {
            NodeObj->SetStringField(TEXT("title"), Row.Title);
        }
        if (!Row.ClassPath.IsEmpty())
        {
            NodeObj->SetStringField(TEXT("class"), Row.ClassPath);
        }
        if (!Row.ExistingComment.IsEmpty())
        {
            NodeObj->SetStringField(TEXT("comment"), Row.ExistingComment);
        }

        NodeValues.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    RootObj->SetArrayField(TEXT("nodes"), NodeValues);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    FJsonSerializer::Serialize(RootObj, Writer);
    return !OutJson.IsEmpty();
}

bool BEP_NodeJson::SaveCommentJsonToFile(const FString& Json, const FString& SuggestedFileStem, const FBEP_NodeJsonExportOptions& Opt, FString& OutAbsPath, FString* OutTemplateCsvPath, FString* OutError, const TArray<FString>* TemplateGuids, bool bGoldenSample)
{
    OutAbsPath.Reset();

    const FString FilePath = BuildFilePath(SuggestedFileStem, TEXT(".json"), bGoldenSample);
    const bool bSaved = FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    if (!bSaved)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("Failed to save file: %s"), *FilePath);
        }
        return false;
    }

    OutAbsPath = FPaths::ConvertRelativePathToFull(FilePath);

    if (OutTemplateCsvPath && TemplateGuids && TemplateGuids->Num() > 0)
    {
        const FString TemplatePath = BuildFilePath(SuggestedFileStem, TEXT(".template.csv"), bGoldenSample);
        FString CsvContent = TEXT("node_guid,comment\n");
        for (const FString& Guid : *TemplateGuids)
        {
            CsvContent += FString::Printf(TEXT("%s,\n"), *Guid);
        }
        FFileHelper::SaveStringToFile(CsvContent, *TemplatePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        *OutTemplateCsvPath = FPaths::ConvertRelativePathToFull(TemplatePath);
    }

    return true;
}

bool BEP_NodeJson::WriteGoldenSamples(FString& OutGoldenSummaryPath, FString* OutError, FString* OutSuperCompactPath, FString* OutAuditPath, FString* OutCommentPath, FString* OutTemplatePath)
{
    OutGoldenSummaryPath.Reset();
    if (OutSuperCompactPath) { OutSuperCompactPath->Reset(); }
    if (OutAuditPath) { OutAuditPath->Reset(); }
    if (OutCommentPath) { OutCommentPath->Reset(); }
    if (OutTemplatePath) { OutTemplatePath->Reset(); }

    const UBEP_NodeJsonExportSettings* Settings = GetDefault<UBEP_NodeJsonExportSettings>();
    FBEP_NodeJsonExportOptions BaseOpt;
    if (Settings)
    {
        Settings->BuildExportOptions(BaseOpt);
    }
    else
    {
        ApplyPreset(BaseOpt, EBEP_NodeJsonPreset::SuperCompact);
    }

    auto ExportPreset = [&](EBEP_NodeJsonPreset Preset, const TCHAR* FixedStem, FString& OutPath, FString& OutSummary, FBEP_NodeJsonExportStats& OutStats) -> bool
    {
        FBEP_NodeJsonExportOptions Opt = BaseOpt;
        Opt.Preset = Preset;
        ApplyPreset(Opt, Preset);

        FString Json;
        FString SuggestedStem;
        FString Error;
        int32 NodeCount = 0;
        int32 EdgeCount = 0;

        if (!ExportActiveSelectionToJson(Opt, Json, SuggestedStem, NodeCount, EdgeCount, &Error, &OutStats, false))
        {
            if (OutError) { *OutError = Error; }
            UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Golden sample export failed (%s): %s"), *PresetToString(Preset), *Error);
            return false;
        }

        if (!SaveJsonToFile(Json, FixedStem, Opt, &OutStats, OutPath, &OutSummary, &Error, true))
        {
            if (OutError) { *OutError = Error; }
            UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Golden sample save failed (%s): %s"), *PresetToString(Preset), *Error);
            return false;
        }

        UE_LOG(LogBEPNodeJson, Log, TEXT("[BEP][NodeJSON] Golden sample saved (%s): %s"), *PresetToString(Preset), *OutPath);
        return true;
    };

    FBEP_NodeJsonExportStats SuperStats;
    FString SuperPath;
    FString SuperSummary;
    if (!ExportPreset(EBEP_NodeJsonPreset::SuperCompact, TEXT("Golden_SuperCompact"), SuperPath, SuperSummary, SuperStats))
    {
        return false;
    }

    FBEP_NodeJsonExportStats AuditStats;
    FString AuditPath;
    FString AuditSummary;
    if (!ExportPreset(EBEP_NodeJsonPreset::AIAudit, TEXT("Golden_AIAudit"), AuditPath, AuditSummary, AuditStats))
    {
        return false;
    }

    FString CommentJson;
    FString CommentStem;
    FString CommentError;
    TArray<FString> CommentGuids;
    if (!ExportActiveSelectionToCommentJson(BaseOpt, CommentJson, CommentStem, &CommentGuids, &CommentError))
    {
        if (OutError) { *OutError = CommentError; }
        UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Golden sample comment export failed: %s"), *CommentError);
        return false;
    }

    FString CommentPath;
    FString TemplatePath;
    if (!SaveCommentJsonToFile(CommentJson, TEXT("Golden_Comments"), BaseOpt, CommentPath, &TemplatePath, &CommentError, &CommentGuids, true))
    {
        if (OutError) { *OutError = CommentError; }
        UE_LOG(LogBEPNodeJson, Warning, TEXT("[BEP][NodeJSON] Golden sample comment save failed: %s"), *CommentError);
        return false;
    }

    const FString GoldenDir = FPaths::GetPath(SuperPath.IsEmpty() ? AuditPath : SuperPath);
    const FString GoldenDirFull = FPaths::ConvertRelativePathToFull(GoldenDir);
    IFileManager::Get().MakeDirectory(*GoldenDirFull, true);

    FString GoldenSummary;
    GoldenSummary += FString::Printf(TEXT("Golden Samples Summary (UTC): %s\n"), *FDateTime::UtcNow().ToIso8601());
    GoldenSummary += FString::Printf(TEXT("OutputDir: %s\n\n"), *GoldenDirFull);

    auto AppendStats = [&](const TCHAR* Label, const FString& Path, const FString& SummPath, const FBEP_NodeJsonExportStats& Stats)
    {
        GoldenSummary += FString::Printf(TEXT("%s:\n"), Label);
        GoldenSummary += FString::Printf(TEXT("  Path: %s\n"), *Path);
        if (!SummPath.IsEmpty())
        {
            GoldenSummary += FString::Printf(TEXT("  File Summary: %s\n"), *SummPath);
        }
        GoldenSummary += FString::Printf(TEXT("  Seed: %d, Expanded: %d, FinalNodes: %d (cap %d hit=%s)\n"),
            Stats.SeedSelectionCount,
            Stats.ExpandedNodeCount,
            Stats.FinalNodeCount,
            Stats.MaxNodeCap,
            Stats.bHitNodeCap ? TEXT("true") : TEXT("false"));
        GoldenSummary += FString::Printf(TEXT("  Edges: Exec %d / Data %d (total %d, cap %d hit=%s)\n"),
            Stats.ExecEdgeCount,
            Stats.DataEdgeCount,
            Stats.ExecEdgeCount + Stats.DataEdgeCount,
            Stats.MaxEdgeCap,
            Stats.bHitEdgeCap ? TEXT("true") : TEXT("false"));
        GoldenSummary += FString::Printf(TEXT("  ExecDepth: %d, DataDepth: %d\n\n"), Stats.ExecDepthUsed, Stats.DataDepthUsed);
    };

    AppendStats(TEXT("SuperCompact"), SuperPath, SuperSummary, SuperStats);
    AppendStats(TEXT("AIAudit"), AuditPath, AuditSummary, AuditStats);

    GoldenSummary += FString::Printf(TEXT("Comments:\n  JSON: %s\n  Template: %s\n  GUIDs: %d\n"), *CommentPath, *TemplatePath, CommentGuids.Num());

    const FString GoldenSummaryPath = FPaths::Combine(GoldenDirFull, TEXT("GoldenSamples.summary.txt"));
    FFileHelper::SaveStringToFile(GoldenSummary, *GoldenSummaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    OutGoldenSummaryPath = GoldenSummaryPath;

    if (OutSuperCompactPath) { *OutSuperCompactPath = SuperPath; }
    if (OutAuditPath) { *OutAuditPath = AuditPath; }
    if (OutCommentPath) { *OutCommentPath = CommentPath; }
    if (OutTemplatePath) { *OutTemplatePath = TemplatePath; }

    UE_LOG(LogBEPNodeJson, Log, TEXT("[BEP][NodeJSON] Golden samples complete: %s"), *GoldenSummaryPath);
    return true;
}

bool BEP_NodeJson::HasActiveBlueprintSelection(FString* OutReason)
{
    const TSharedPtr<SGraphEditor> GraphEditor = FindActiveGraphEditor();
    UEdGraph* Graph = GetGraphFromEditor(GraphEditor);
    if (!Graph)
    {
        if (OutReason) { *OutReason = TEXT("No active graph editor."); }
        return false;
    }

    const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
    if (Selection.Num() == 0)
    {
        if (OutReason) { *OutReason = TEXT("No nodes selected."); }
        return false;
    }

    return true;
}

bool BEP_NodeJson::HasActiveBlueprintGraph(FString* OutReason)
{
    const TSharedPtr<SGraphEditor> GraphEditor = FindActiveGraphEditor();
    UEdGraph* Graph = GetGraphFromEditor(GraphEditor);
    if (!Graph)
    {
        if (OutReason) { *OutReason = TEXT("No active graph editor."); }
        return false;
    }
    return true;
}

bool BEP_NodeJson::RunSelfCheck(FString& OutReport)
{
    TArray<FString> Issues;
    TArray<FString> Notes;

    FString SelectionReason;
    if (!HasActiveBlueprintSelection(&SelectionReason))
    {
        Issues.Add(FString::Printf(TEXT("Selection: %s"), *SelectionReason));
    }
    else
    {
        Notes.Add(TEXT("Selection detected."));
    }

    const FString Root = GetBEPExportRoot();
    if (Root.IsEmpty())
    {
        Issues.Add(TEXT("Export root is empty (check BEP settings)."));
    }
    else
    {
        const bool bDirOk = IFileManager::Get().MakeDirectory(*Root, true);
        if (!bDirOk)
        {
            Issues.Add(FString::Printf(TEXT("Export root not writable: %s"), *Root));
        }
        else
        {
            Notes.Add(FString::Printf(TEXT("Export root OK: %s"), *Root));
        }
    }

    auto CheckModule = [&](const TCHAR* Name)
    {
        if (!FModuleManager::Get().IsModuleLoaded(Name) && !FModuleManager::Get().GetModulePtr<IModuleInterface>(Name))
        {
            Issues.Add(FString::Printf(TEXT("Module missing: %s"), Name));
        }
        else
        {
            Notes.Add(FString::Printf(TEXT("Module loaded: %s"), Name));
        }
    };

    CheckModule(TEXT("ToolMenus"));
    CheckModule(TEXT("PropertyEditor"));
    CheckModule(TEXT("DesktopPlatform"));

    const UBEP_NodeJsonExportSettings* Settings = GetDefault<UBEP_NodeJsonExportSettings>();
    if (Settings)
    {
        if (Settings->MaxNodesHardCap < 1)
        {
            Issues.Add(TEXT("MaxNodesHardCap < 1"));
        }
        if (Settings->MaxEdgesHardCap < 1)
        {
            Issues.Add(TEXT("MaxEdgesHardCap < 1"));
        }
        Notes.Add(FString::Printf(TEXT("Caps: Nodes=%d Edges=%d Warn=%s"), Settings->MaxNodesHardCap, Settings->MaxEdgesHardCap, Settings->bWarnOnCapHit ? TEXT("true") : TEXT("false")));
    }
    else
    {
        Issues.Add(TEXT("Settings object unavailable."));
    }

    OutReport.Reset();
    OutReport += FString::Printf(TEXT("BEP NodeJSON Self-Check (UTC %s)\n"), *FDateTime::UtcNow().ToIso8601());
    OutReport += TEXT("Notes:\n");
    for (const FString& Note : Notes)
    {
        OutReport += TEXT(" - ") + Note + TEXT("\n");
    }

    if (Issues.Num() == 0)
    {
        OutReport += TEXT("No blocking issues detected.\n");
        return true;
    }

    OutReport += TEXT("Issues:\n");
    for (const FString& Issue : Issues)
    {
        OutReport += TEXT(" - ") + Issue + TEXT("\n");
    }

    return false;
}
