#include "SOTS_BPGenBuilder.h"
#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenSpawnerRegistry.h"
#include "SOTS_BPGenDescriptorTranslator.h"
#include "SOTS_BPGenGraphResolver.h"
#include "SOTS_BPGenEditGuard.h"

#include "BlueprintGraphModule.h"
#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeBinder.h"
#include "BlueprintVariableNodeSpawner.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedEnum.h"
#include "Interfaces/IPluginManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Variable.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

// Global mutex defined here (declared in SOTS_BPGenEditGuard.h).
FCriticalSection GSOTS_BPGenEditMutex;

struct FSOTS_BPGenAutoFixState;

static UEdGraphPin* FindPinByName(UEdGraphNode* Node, FName PinName);
static UEdGraphPin* FindPinHeuristic(UEdGraphNode* Node, FName PinName);
static UEdGraphNode* SpawnNodeFromSpawnerKey(UBlueprint* Blueprint, UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, FSOTS_BPGenApplyResult& Result);
static void ApplyGraphLinks(UBlueprint* Blueprint, UEdGraph* Graph, const FSOTS_BPGenGraphSpec& GraphSpec, const TMap<FString, UEdGraphNode*>& NodeMap, const TSet<FString>& SkippedSpecIds, FSOTS_BPGenApplyResult& Result);
static FString GetBPGenNodeId(const UEdGraphNode* Node);
static void SetBPGenNodeId(UEdGraphNode* Node, const FString& NodeId);
static FString NormalizeTargetType(const FString& InType);

static const int32 GSpecCurrentVersion = 1;
static const FString GSpecCurrentSchema(TEXT("SOTS_BPGen_GraphSpec"));

enum class EBPGenRepairMode : uint8
{
	None,
	Soft,
	Aggressive
};

struct FBPGenPinAliasEntry
{
	FString NodeClass;
	FString FunctionPath;
	TMap<FString, TArray<FString>> PinAliases;
};

struct FBPGenMigrationMaps
{
	bool bLoaded = false;
	FString LoadError;
	TArray<FBPGenPinAliasEntry> PinAliases;
	TMap<FString, FString> NodeClassAliases;
	TMap<FString, FString> FunctionPathAliases;
};

static FString NormalizeKey(const FString& InValue)
{
	FString Out = InValue;
	Out.TrimStartAndEndInline();
	Out = Out.ToLower();
	return Out;
}

static FString ExtractClassNameFromPath(const FString& InPath)
{
	FString Base = InPath;
	int32 PipeIndex = INDEX_NONE;
	if (Base.FindChar(TEXT('|'), PipeIndex))
	{
		Base = Base.Left(PipeIndex);
	}

	int32 DotIndex = INDEX_NONE;
	if (Base.FindLastChar(TEXT('.'), DotIndex))
	{
		Base = Base.Mid(DotIndex + 1);
	}

	int32 SlashIndex = INDEX_NONE;
	if (Base.FindLastChar(TEXT('/'), SlashIndex))
	{
		Base = Base.Mid(SlashIndex + 1);
	}

	return Base;
}

static FString ResolveNodeClassForSpec(const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!NodeSpec.NodeType.IsNone())
	{
		const FString NodeTypeString = NodeSpec.NodeType.ToString();
		if (NodeTypeString.StartsWith(TEXT("K2Node_")))
		{
			return NodeTypeString;
		}

		const FString Lower = NodeTypeString.ToLower();
		if (Lower == TEXT("function_call"))
		{
			return TEXT("K2Node_CallFunction");
		}
		if (Lower == TEXT("variable_get"))
		{
			return TEXT("K2Node_VariableGet");
		}
		if (Lower == TEXT("variable_set"))
		{
			return TEXT("K2Node_VariableSet");
		}
		if (Lower == TEXT("cast"))
		{
			return TEXT("K2Node_DynamicCast");
		}
		if (Lower == TEXT("knot"))
		{
			return TEXT("K2Node_Knot");
		}
		if (Lower == TEXT("select"))
		{
			return TEXT("K2Node_Select");
		}

		if (NodeTypeString.Contains(TEXT("/")) || NodeTypeString.Contains(TEXT(".")) || NodeTypeString.Contains(TEXT("|")))
		{
			return ExtractClassNameFromPath(NodeTypeString);
		}

		return NodeTypeString;
	}

	if (!NodeSpec.SpawnerKey.IsEmpty())
	{
		if (NodeSpec.SpawnerKey.StartsWith(TEXT("/Script/")) && NodeSpec.SpawnerKey.Contains(TEXT(":")))
		{
			return TEXT("K2Node_CallFunction");
		}

		const FString ClassName = ExtractClassNameFromPath(NodeSpec.SpawnerKey);
		if (!ClassName.IsEmpty())
		{
			return ClassName;
		}
	}

	return FString();
}

static FString ResolveFunctionPathForSpec(const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!NodeSpec.FunctionPath.IsEmpty())
	{
		return NodeSpec.FunctionPath;
	}

	if (!NodeSpec.SpawnerKey.IsEmpty() && NodeSpec.SpawnerKey.StartsWith(TEXT("/Script/")) && NodeSpec.SpawnerKey.Contains(TEXT(":")))
	{
		return NodeSpec.SpawnerKey;
	}

	return FString();
}

static FString SanitizeIdSegment(const FString& InValue)
{
	FString Out;
	Out.Reserve(InValue.Len());
	for (const TCHAR Ch : InValue)
	{
		if (FChar::IsAlnum(Ch))
		{
			Out.AppendChar(Ch);
		}
		else
		{
			Out.AppendChar(TEXT('_'));
		}
	}

	Out.TrimStartAndEndInline();
	return Out.IsEmpty() ? TEXT("node") : Out;
}

static FString MakeDeterministicNodeId(const FSOTS_BPGenGraphNode& NodeSpec, int32 Index)
{
	FString Base = !NodeSpec.SpawnerKey.IsEmpty() ? NodeSpec.SpawnerKey : NodeSpec.NodeType.ToString();
	if (Base.IsEmpty())
	{
		Base = TEXT("node");
	}

	const FString Safe = SanitizeIdSegment(Base);
	return FString::Printf(TEXT("auto_%s_%d"), *Safe, Index);
}

static bool LoadJsonArrayFile(const FString& FilePath, TArray<TSharedPtr<FJsonValue>>& OutValues, FString& OutError)
{
	OutValues.Reset();
	OutError.Reset();

	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *FilePath))
	{
		OutError = FString::Printf(TEXT("Failed to read migration map '%s'."), *FilePath);
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	TSharedPtr<FJsonValue> RootValue;
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		OutError = FString::Printf(TEXT("Failed to parse JSON in '%s'."), *FilePath);
		return false;
	}

	if (RootValue->Type != EJson::Array)
	{
		OutError = FString::Printf(TEXT("Expected JSON array in '%s'."), *FilePath);
		return false;
	}

	OutValues = RootValue->AsArray();
	return true;
}

static void LoadMigrationMaps(FBPGenMigrationMaps& Maps)
{
	Maps.bLoaded = true;
	Maps.LoadError.Reset();
	Maps.PinAliases.Reset();
	Maps.NodeClassAliases.Reset();
	Maps.FunctionPathAliases.Reset();

	FString PluginDir;
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SOTS_BlueprintGen")))
	{
		PluginDir = Plugin->GetBaseDir();
	}
	else
	{
		PluginDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("SOTS_BlueprintGen"));
	}

	const FString MigrationDir = FPaths::Combine(PluginDir, TEXT("Content"), TEXT("BPGenMigrations"));
	const FString PinAliasesPath = FPaths::Combine(MigrationDir, TEXT("pin_aliases.json"));
	const FString NodeClassAliasesPath = FPaths::Combine(MigrationDir, TEXT("node_class_aliases.json"));
	const FString FunctionPathAliasesPath = FPaths::Combine(MigrationDir, TEXT("function_path_aliases.json"));

	auto RecordError = [&](const FString& Error)
	{
		if (!Error.IsEmpty())
		{
			if (!Maps.LoadError.IsEmpty())
			{
				Maps.LoadError += TEXT(" ");
			}
			Maps.LoadError += Error;
		}
	};

	if (FPaths::FileExists(PinAliasesPath))
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		FString Error;
		if (LoadJsonArrayFile(PinAliasesPath, Values, Error))
		{
			for (const TSharedPtr<FJsonValue>& Value : Values)
			{
				const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
				if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
				{
					continue;
				}

				FBPGenPinAliasEntry Entry;
				(*ObjPtr)->TryGetStringField(TEXT("node_class"), Entry.NodeClass);
				(*ObjPtr)->TryGetStringField(TEXT("function_path"), Entry.FunctionPath);
				Entry.NodeClass = NormalizeKey(Entry.NodeClass);
				Entry.FunctionPath = NormalizeKey(Entry.FunctionPath);

				const TSharedPtr<FJsonObject>* AliasObj = nullptr;
				if ((*ObjPtr)->TryGetObjectField(TEXT("pin_aliases"), AliasObj) && AliasObj && AliasObj->IsValid())
				{
					for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*AliasObj)->Values)
					{
						TArray<FString> Aliases;
						const TArray<TSharedPtr<FJsonValue>>* AliasArray = nullptr;
						if (Pair.Value.IsValid() && Pair.Value->TryGetArray(AliasArray) && AliasArray)
						{
							for (const TSharedPtr<FJsonValue>& AliasValue : *AliasArray)
							{
								FString AliasStr;
								if (AliasValue.IsValid() && AliasValue->TryGetString(AliasStr))
								{
									Aliases.Add(AliasStr);
								}
							}
						}
						Entry.PinAliases.Add(Pair.Key, Aliases);
					}
				}

				if (!Entry.NodeClass.IsEmpty() || !Entry.FunctionPath.IsEmpty())
				{
					Maps.PinAliases.Add(MoveTemp(Entry));
				}
			}
		}
		else
		{
			RecordError(Error);
		}
	}

	auto LoadAliasMap = [&](const FString& FilePath, TMap<FString, FString>& OutMap)
	{
		if (!FPaths::FileExists(FilePath))
		{
			return;
		}

		TArray<TSharedPtr<FJsonValue>> Values;
		FString Error;
		if (!LoadJsonArrayFile(FilePath, Values, Error))
		{
			RecordError(Error);
			return;
		}

		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
			{
				continue;
			}

			FString OldValue;
			FString NewValue;
			(*ObjPtr)->TryGetStringField(TEXT("old"), OldValue);
			(*ObjPtr)->TryGetStringField(TEXT("new"), NewValue);

			const FString OldKey = NormalizeKey(OldValue);
			if (!OldKey.IsEmpty() && !NewValue.IsEmpty())
			{
				OutMap.Add(OldKey, NewValue);
			}
		}
	};

	LoadAliasMap(NodeClassAliasesPath, Maps.NodeClassAliases);
	LoadAliasMap(FunctionPathAliasesPath, Maps.FunctionPathAliases);
}

static const FBPGenMigrationMaps& GetMigrationMaps()
{
	static FBPGenMigrationMaps Maps;
	if (!Maps.bLoaded)
	{
		LoadMigrationMaps(Maps);
	}
	return Maps;
}

static bool TryResolvePinAlias(const FSOTS_BPGenGraphNode& NodeSpec, FName& InOutPinName, const FBPGenMigrationMaps& Maps, FString& OutCanonical)
{
	const FString NodeClassKey = NormalizeKey(ResolveNodeClassForSpec(NodeSpec));
	const FString FunctionPathKey = NormalizeKey(ResolveFunctionPathForSpec(NodeSpec));
	if (NodeClassKey.IsEmpty() || FunctionPathKey.IsEmpty())
	{
		return false;
	}

	const FString Requested = InOutPinName.ToString();
	for (const FBPGenPinAliasEntry& Entry : Maps.PinAliases)
	{
		if (!Entry.NodeClass.IsEmpty() && Entry.NodeClass != NodeClassKey)
		{
			continue;
		}
		if (!Entry.FunctionPath.IsEmpty() && Entry.FunctionPath != FunctionPathKey)
		{
			continue;
		}

		for (const TPair<FString, TArray<FString>>& Pair : Entry.PinAliases)
		{
			const FString Canonical = Pair.Key;
			if (Canonical.IsEmpty())
			{
				continue;
			}

			if (Canonical.Equals(Requested, ESearchCase::IgnoreCase))
			{
				return false;
			}

			for (const FString& Alias : Pair.Value)
			{
				if (Alias.Equals(Requested, ESearchCase::IgnoreCase))
				{
					InOutPinName = FName(*Canonical);
					OutCanonical = Canonical;
					return true;
				}
			}
		}
	}

	return false;
}

static void ApplySpecMigrations(FSOTS_BPGenGraphSpec& Spec, TArray<FString>& OutMigrationNotes, bool& bSpecMigrated)
{
	const FBPGenMigrationMaps& Maps = GetMigrationMaps();
	if (!Maps.LoadError.IsEmpty())
	{
		OutMigrationNotes.Add(Maps.LoadError);
	}

	for (FSOTS_BPGenGraphNode& NodeSpec : Spec.Nodes)
	{
		if (!NodeSpec.NodeType.IsNone())
		{
			const FString NodeTypeString = NodeSpec.NodeType.ToString();
			const FString NodeKey = NormalizeKey(NodeTypeString);
			const FString* NewClass = Maps.NodeClassAliases.Find(NodeKey);
			if (!NewClass)
			{
				const FString Extracted = ExtractClassNameFromPath(NodeTypeString);
				const FString ExtractedKey = NormalizeKey(Extracted);
				NewClass = Maps.NodeClassAliases.Find(ExtractedKey);
			}

			if (NewClass && !NewClass->IsEmpty() && !NodeTypeString.Equals(*NewClass, ESearchCase::IgnoreCase))
			{
				NodeSpec.NodeType = FName(**NewClass);
				bSpecMigrated = true;
				OutMigrationNotes.Add(FString::Printf(TEXT("Node class migrated: %s -> %s."), *NodeTypeString, **NewClass));
			}
		}

		const FString OldFunctionPath = ResolveFunctionPathForSpec(NodeSpec);
		const FString OldKey = NormalizeKey(OldFunctionPath);
		if (!OldKey.IsEmpty())
		{
			if (const FString* NewPath = Maps.FunctionPathAliases.Find(OldKey))
			{
				const FString NewValue = *NewPath;
				if (!NodeSpec.FunctionPath.IsEmpty() && !NodeSpec.FunctionPath.Equals(NewValue, ESearchCase::IgnoreCase))
				{
					OutMigrationNotes.Add(FString::Printf(TEXT("Function path migrated: %s -> %s."), *NodeSpec.FunctionPath, *NewValue));
					NodeSpec.FunctionPath = NewValue;
					bSpecMigrated = true;
				}

				if (!NodeSpec.SpawnerKey.IsEmpty() && NodeSpec.SpawnerKey.Equals(OldFunctionPath, ESearchCase::IgnoreCase))
				{
					NodeSpec.SpawnerKey = NewValue;
					bSpecMigrated = true;
				}

				if (NodeSpec.FunctionPath.IsEmpty() && NodeSpec.SpawnerKey.Equals(NewValue, ESearchCase::IgnoreCase))
				{
					NodeSpec.FunctionPath = NewValue;
				}
			}
		}
	}

	TMap<FString, const FSOTS_BPGenGraphNode*> NodeById;
	NodeById.Reserve(Spec.Nodes.Num());
	for (const FSOTS_BPGenGraphNode& NodeSpec : Spec.Nodes)
	{
		NodeById.Add(NodeSpec.Id, &NodeSpec);
	}

	for (FSOTS_BPGenGraphLink& Link : Spec.Links)
	{
		if (const FSOTS_BPGenGraphNode* FromSpec = NodeById.FindRef(Link.FromNodeId))
		{
			const FName Before = Link.FromPinName;
			FString Canonical;
			if (TryResolvePinAlias(*FromSpec, Link.FromPinName, Maps, Canonical))
			{
				bSpecMigrated = true;
				OutMigrationNotes.Add(FString::Printf(TEXT("Pin alias migrated on %s: %s -> %s."), *Link.FromNodeId, *Before.ToString(), *Link.FromPinName.ToString()));
			}
		}

		if (const FSOTS_BPGenGraphNode* ToSpec = NodeById.FindRef(Link.ToNodeId))
		{
			const FName Before = Link.ToPinName;
			FString Canonical;
			if (TryResolvePinAlias(*ToSpec, Link.ToPinName, Maps, Canonical))
			{
				bSpecMigrated = true;
				OutMigrationNotes.Add(FString::Printf(TEXT("Pin alias migrated on %s: %s -> %s."), *Link.ToNodeId, *Before.ToString(), *Link.ToPinName.ToString()));
			}
		}
	}
}

static void SortNodesDeterministic(TArray<FSOTS_BPGenGraphNode>& Nodes)
{
	Nodes.Sort([](const FSOTS_BPGenGraphNode& A, const FSOTS_BPGenGraphNode& B)
	{
		const bool bAHasId = !A.NodeId.IsEmpty();
		const bool bBHasId = !B.NodeId.IsEmpty();
		if (bAHasId != bBHasId)
		{
			return bAHasId;
		}

		int32 Cmp = A.NodeId.Compare(B.NodeId);
		if (Cmp != 0)
		{
			return Cmp < 0;
		}

		const FString ClassA = ResolveNodeClassForSpec(A);
		const FString ClassB = ResolveNodeClassForSpec(B);
		Cmp = ClassA.Compare(ClassB);
		if (Cmp != 0)
		{
			return Cmp < 0;
		}

		if (A.NodePosition.Y != B.NodePosition.Y)
		{
			return A.NodePosition.Y < B.NodePosition.Y;
		}

		return A.NodePosition.X < B.NodePosition.X;
	});
}

static void SortLinksDeterministic(TArray<FSOTS_BPGenGraphLink>& Links)
{
	Links.Sort([](const FSOTS_BPGenGraphLink& A, const FSOTS_BPGenGraphLink& B)
	{
		int32 Cmp = A.FromNodeId.Compare(B.FromNodeId);
		if (Cmp != 0)
		{
			return Cmp < 0;
		}

		Cmp = A.FromPinName.Compare(B.FromPinName);
		if (Cmp != 0)
		{
			return Cmp < 0;
		}

		Cmp = A.ToNodeId.Compare(B.ToNodeId);
		if (Cmp != 0)
		{
			return Cmp < 0;
		}

		return A.ToPinName.Compare(B.ToPinName) < 0;
	});
}

static void CanonicalizeGraphSpecInternal(FSOTS_BPGenGraphSpec& Spec, const FSOTS_BPGenSpecCanonicalizeOptions& Options, TArray<FString>& OutDiffNotes, TArray<FString>& OutMigrationNotes, bool& bSpecMigrated)
{
	OutDiffNotes.Reset();
	OutMigrationNotes.Reset();
	bSpecMigrated = false;

	if (Spec.SpecVersion <= 0)
	{
		OutMigrationNotes.Add(FString::Printf(TEXT("Spec version defaulted to %d."), GSpecCurrentVersion));
		Spec.SpecVersion = GSpecCurrentVersion;
		bSpecMigrated = true;
	}
	else if (Spec.SpecVersion < GSpecCurrentVersion)
	{
		OutMigrationNotes.Add(FString::Printf(TEXT("Spec version upgraded: %d -> %d."), Spec.SpecVersion, GSpecCurrentVersion));
		Spec.SpecVersion = GSpecCurrentVersion;
		bSpecMigrated = true;
	}
	else if (Spec.SpecVersion > GSpecCurrentVersion)
	{
		OutDiffNotes.Add(FString::Printf(TEXT("Spec version %d is newer than supported %d; leaving as-is."), Spec.SpecVersion, GSpecCurrentVersion));
	}

	if (!Spec.SpecSchema.Equals(GSpecCurrentSchema, ESearchCase::IgnoreCase))
	{
		const FString Previous = Spec.SpecSchema;
		Spec.SpecSchema = GSpecCurrentSchema;
		OutMigrationNotes.Add(FString::Printf(TEXT("Spec schema normalized: %s -> %s."), *Previous, *Spec.SpecSchema));
		bSpecMigrated = true;
	}

	if (Options.bNormalizeTargetType)
	{
		const FString Before = Spec.Target.TargetType;
		const FString Normalized = NormalizeTargetType(Spec.Target.TargetType.IsEmpty() ? TEXT("Function") : Spec.Target.TargetType);
		if (!Normalized.Equals(Before, ESearchCase::CaseSensitive))
		{
			Spec.Target.TargetType = Normalized;
			OutDiffNotes.Add(FString::Printf(TEXT("Normalized target_type: %s -> %s."), *Before, *Normalized));
		}
	}

	if (Options.bApplyMigrations)
	{
		ApplySpecMigrations(Spec, OutMigrationNotes, bSpecMigrated);
	}

	if (Options.bAssignMissingNodeIds)
	{
		TSet<FString> UsedNodeIds;
		for (const FSOTS_BPGenGraphNode& NodeSpec : Spec.Nodes)
		{
			if (!NodeSpec.NodeId.IsEmpty())
			{
				UsedNodeIds.Add(NodeSpec.NodeId);
			}
		}

		for (int32 Index = 0; Index < Spec.Nodes.Num(); ++Index)
		{
			FSOTS_BPGenGraphNode& NodeSpec = Spec.Nodes[Index];
			if (!NodeSpec.NodeId.IsEmpty() || !NodeSpec.bAllowCreate)
			{
				continue;
			}

			const FString Candidate = MakeDeterministicNodeId(NodeSpec, Index);
			FString Unique = Candidate;
			int32 Suffix = 1;
			while (UsedNodeIds.Contains(Unique))
			{
				Unique = FString::Printf(TEXT("%s_%d"), *Candidate, Suffix++);
			}

			NodeSpec.NodeId = Unique;
			UsedNodeIds.Add(Unique);
			OutDiffNotes.Add(FString::Printf(TEXT("Assigned node_id '%s' to node '%s'."), *Unique, *NodeSpec.Id));
		}
	}

	if (Options.bSortDeterministic)
	{
		SortNodesDeterministic(Spec.Nodes);
		SortLinksDeterministic(Spec.Links);
	}
}

static FSOTS_BPGenGraphSpec CanonicalizeGraphSpecForApply(const FSOTS_BPGenGraphSpec& GraphSpec, FSOTS_BPGenApplyResult& Result)
{
	FSOTS_BPGenGraphSpec Canonical = GraphSpec;
	FSOTS_BPGenSpecCanonicalizeOptions Options;
	Options.bAssignMissingNodeIds = true;
	Options.bSortDeterministic = true;
	Options.bNormalizeTargetType = true;
	Options.bApplyMigrations = true;

	TArray<FString> DiffNotes;
	TArray<FString> MigrationNotes;
	bool bSpecMigrated = false;
	CanonicalizeGraphSpecInternal(Canonical, Options, DiffNotes, MigrationNotes, bSpecMigrated);

	Result.bSpecMigrated = bSpecMigrated;
	Result.MigrationNotes = MigrationNotes;
	return Canonical;
}

static EBPGenRepairMode ParseRepairMode(const FString& InMode)
{
	FString Mode = InMode;
	Mode.TrimStartAndEndInline();
	Mode = Mode.ToLower();
	if (Mode == TEXT("soft"))
	{
		return EBPGenRepairMode::Soft;
	}
	if (Mode == TEXT("aggressive"))
	{
		return EBPGenRepairMode::Aggressive;
	}
	return EBPGenRepairMode::None;
}

static FString GetSpawnerKeyForNode(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	if (const UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
	{
		if (UFunction* Function = CallNode->GetTargetFunction())
		{
			return Function->GetPathName();
		}
	}

	if (const UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(Node))
	{
		const FName VarName = VarNode->GetVarName();
		if (const UClass* OwnerClass = VarNode->GetVariableSourceClass())
		{
			if (VarName != NAME_None)
			{
				return FString::Printf(TEXT("%s:%s"), *OwnerClass->GetPathName(), *VarName.ToString());
			}
		}
	}

	if (const UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
	{
		if (UClass* TargetType = CastNode->TargetType)
		{
			return FString::Printf(TEXT("K2Node_DynamicCast|%s"), *TargetType->GetPathName());
		}
	}

	return Node->GetClass() ? Node->GetClass()->GetPathName() : FString();
}

static bool IsApproxPositionMatch(const FSOTS_BPGenGraphNode& NodeSpec, const UEdGraphNode* Node, int32 Tolerance)
{
	if (!Node)
	{
		return false;
	}

	return FMath::Abs(Node->NodePosX - NodeSpec.NodePosition.X) <= Tolerance
		&& FMath::Abs(Node->NodePosY - NodeSpec.NodePosition.Y) <= Tolerance;
}

static UEdGraphNode* FindRepairCandidate(
	const FSOTS_BPGenGraphNode& NodeSpec,
	const TArray<UEdGraphNode*>& Nodes,
	const TSet<UEdGraphNode*>& ClaimedNodes,
	EBPGenRepairMode Mode,
	FString& OutDescription)
{
	if (Mode == EBPGenRepairMode::None)
	{
		return nullptr;
	}

	const FString DesiredClass = ResolveNodeClassForSpec(NodeSpec);
	FString DesiredSpawnerKey = NodeSpec.SpawnerKey;
	if (DesiredSpawnerKey.IsEmpty() && Mode == EBPGenRepairMode::Aggressive)
	{
		DesiredSpawnerKey = ResolveFunctionPathForSpec(NodeSpec);
	}

	if (Mode == EBPGenRepairMode::Soft && DesiredSpawnerKey.IsEmpty())
	{
		return nullptr;
	}

	const int32 Tolerance = (Mode == EBPGenRepairMode::Aggressive) ? 96 : 32;

	for (UEdGraphNode* Node : Nodes)
	{
		if (!Node || ClaimedNodes.Contains(Node))
		{
			continue;
		}

		if (!GetBPGenNodeId(Node).IsEmpty())
		{
			continue;
		}

		if (!DesiredClass.IsEmpty() && !Node->GetClass()->GetName().Equals(DesiredClass, ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (!DesiredSpawnerKey.IsEmpty())
		{
			const FString CandidateSpawnerKey = GetSpawnerKeyForNode(Node);
			if (!CandidateSpawnerKey.Equals(DesiredSpawnerKey, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		if (!IsApproxPositionMatch(NodeSpec, Node, Tolerance))
		{
			continue;
		}

		OutDescription = FString::Printf(TEXT("Repair matched node '%s' using %s mode."), *Node->GetName(), Mode == EBPGenRepairMode::Aggressive ? TEXT("aggressive") : TEXT("soft"));
		return Node;
	}

	return nullptr;
}

static FSOTS_BPGenApplyResult ApplyGraphSpecInternal(const UObject* WorldContextObject, const FSOTS_BPGenGraphTarget& Target, const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult Result;
	Result.TargetBlueprintPath = Target.BlueprintAssetPath;
	Result.FunctionName = Target.Name.IsEmpty() ? NAME_None : FName(*Target.Name);

	if (Target.BlueprintAssetPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorCode = TEXT("ERR_INVALID_INPUT");
		Result.ErrorMessage = TEXT("ApplyGraphSpec: BlueprintAssetPath is empty.");
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "ApplyGraphSpec", "SOTS BPGen: Apply Graph"));

	EnsureBlueprintNodeModulesLoaded();

	UBlueprint* Blueprint = nullptr;
	UEdGraph* TargetGraph = nullptr;
	if (!ResolveGraphForApply(Target, Blueprint, TargetGraph, Result))
	{
		return Result;
	}

	const FString NormalizedTargetType = NormalizeTargetType(Target.TargetType);
	const bool bFunctionLike = NormalizedTargetType == TEXT("FUNCTION") || NormalizedTargetType == TEXT("WIDGETBINDING");

	// Identify existing nodes for reuse.
	UK2Node_FunctionEntry* EntryNode = nullptr;
	TArray<UK2Node_FunctionResult*> ResultNodes;
	TMap<FString, UEdGraphNode*> ExistingNodeIdMap;
	const EBPGenRepairMode RepairMode = ParseRepairMode(GraphSpec.RepairMode);
	TSet<UEdGraphNode*> ClaimedRepairNodes;

	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (bFunctionLike)
		{
			if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
			{
				if (!EntryNode)
				{
					EntryNode = AsEntry;
				}
			}
			else if (UK2Node_FunctionResult* AsResult = Cast<UK2Node_FunctionResult>(Node))
			{
				ResultNodes.Add(AsResult);
			}
		}

		const FString ExistingId = GetBPGenNodeId(Node);
		if (!ExistingId.IsEmpty())
		{
			ExistingNodeIdMap.Add(ExistingId, Node);
		}
	}

	if (bFunctionLike)
	{
		if (!EntryNode)
		{
			EntryNode = SpawnFunctionEntryNode(TargetGraph, FVector2D::ZeroVector);
		}

		if (ResultNodes.Num() == 0)
		{
			if (UK2Node_FunctionResult* NewResult = SpawnFunctionResultNode(TargetGraph, FVector2D::ZeroVector))
			{
				ResultNodes.Add(NewResult);
			}
		}
	}

	TMap<FString, UEdGraphNode*> NodeMap;
	TSet<FString> SkippedSpecIds;
	TSet<FString> SkippedNodeIds;

	if (bFunctionLike)
	{
		AddNodeToMap(NodeMap, TEXT("Entry"), EntryNode);
		AddNodeToMap(NodeMap, TEXT("FunctionEntry"), EntryNode);
		if (ResultNodes.Num() > 0)
		{
			AddNodeToMap(NodeMap, TEXT("Result"), ResultNodes[0]);
			AddNodeToMap(NodeMap, TEXT("FunctionResult"), ResultNodes[0]);
		}
		for (int32 Index = 0; Index < ResultNodes.Num(); ++Index)
		{
			AddNodeToMap(NodeMap, FString::Printf(TEXT("Result%d"), Index), ResultNodes[Index]);
		}
	}

	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpec.Nodes)
	{
		if (bFunctionLike && NodeSpec.NodeType == FName(TEXT("K2Node_FunctionEntry")))
		{
			if (EntryNode)
			{
				AddNodeToMap(NodeMap, NodeSpec.Id, EntryNode);
				EntryNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				EntryNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(EntryNode, NodeSpec);

				if (!NodeSpec.NodeId.IsEmpty())
				{
					SetBPGenNodeId(EntryNode, NodeSpec.NodeId);
					Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}
			continue;
		}

		if (bFunctionLike && NodeSpec.NodeType == FName(TEXT("K2Node_FunctionResult")))
		{
			if (ResultNodes.Num() > 0)
			{
				AddNodeToMap(NodeMap, NodeSpec.Id, ResultNodes[0]);
				ResultNodes[0]->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				ResultNodes[0]->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(ResultNodes[0], NodeSpec);

				if (!NodeSpec.NodeId.IsEmpty())
				{
					SetBPGenNodeId(ResultNodes[0], NodeSpec.NodeId);
					Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}
			continue;
		}
	}

	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpec.Nodes)
	{
		if (NodeMap.Contains(NodeSpec.Id))
		{
			continue;
		}

		UEdGraphNode* NewNode = nullptr;
		const bool bHasNodeId = !NodeSpec.NodeId.IsEmpty();
		UEdGraphNode* ExistingNode = bHasNodeId ? ExistingNodeIdMap.FindRef(NodeSpec.NodeId) : nullptr;
		if (!ExistingNode && bHasNodeId && RepairMode != EBPGenRepairMode::None && NodeSpec.bCreateOrUpdate && NodeSpec.bAllowUpdate)
		{
			FString RepairDescription;
			ExistingNode = FindRepairCandidate(NodeSpec, TargetGraph->Nodes, ClaimedRepairNodes, RepairMode, RepairDescription);
			if (ExistingNode)
			{
				SetBPGenNodeId(ExistingNode, NodeSpec.NodeId);
				ExistingNodeIdMap.Add(NodeSpec.NodeId, ExistingNode);
				ClaimedRepairNodes.Add(ExistingNode);

				FSOTS_BPGenRepairStep Step;
				Step.StepIndex = Result.RepairSteps.Num();
				Step.Code = TEXT("REPAIR_NODE_ID");
				Step.Description = RepairDescription;
				Step.AffectedNodeIds.Add(NodeSpec.NodeId);
				Step.Before = TEXT("node_id missing");
				Step.After = NodeSpec.NodeId;
				Result.RepairSteps.Add(Step);
			}
		}
		const bool bAllowCreate = NodeSpec.bAllowCreate;
		const bool bAllowUpdate = NodeSpec.bAllowUpdate;
		const bool bCreateOrUpdate = NodeSpec.bCreateOrUpdate;
		const bool bReuseExistingNode = ExistingNode && bCreateOrUpdate && bAllowUpdate;

		if (bReuseExistingNode)
		{
			NewNode = ExistingNode;
			NewNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
			NewNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
			ApplyExtraPinDefaults(NewNode, NodeSpec);

			if (bHasNodeId)
			{
				SetBPGenNodeId(NewNode, NodeSpec.NodeId);
				Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
			}
		}
		else if (ExistingNode && bCreateOrUpdate && !bAllowUpdate)
		{
			if (bHasNodeId)
			{
				Result.SkippedNodeIds.AddUnique(NodeSpec.NodeId);
				SkippedNodeIds.Add(NodeSpec.NodeId);
			}

			SkippedSpecIds.Add(NodeSpec.Id);
			continue;
		}

		if (!NewNode && (!bAllowCreate && !bHasNodeId))
		{
			if (bHasNodeId)
			{
				Result.SkippedNodeIds.AddUnique(NodeSpec.NodeId);
				SkippedNodeIds.Add(NodeSpec.NodeId);
			}

			SkippedSpecIds.Add(NodeSpec.Id);
			continue;
		}

		if (!NewNode && ExistingNode && !bCreateOrUpdate)
		{
			Result.Warnings.Add(FString::Printf(
				TEXT("NodeId '%s' already exists but create_or_update=false; spawning a new node and leaving the existing one untouched."),
				*NodeSpec.NodeId));
		}

		if (!NewNode)
		{
			const bool bHasSpawnerKey = !NodeSpec.SpawnerKey.IsEmpty();
			if (bHasSpawnerKey && NodeSpec.bPreferSpawnerKey)
			{
				NewNode = SpawnNodeFromSpawnerKey(Blueprint, TargetGraph, NodeSpec, Result);
			}

			if (!NewNode)
			{
				if (NodeSpec.NodeType == FName(TEXT("K2Node_Knot")))
				{
					NewNode = SpawnKnotNode(TargetGraph, NodeSpec);
				}
				else if (NodeSpec.NodeType == FName(TEXT("K2Node_Select")))
				{
					NewNode = SpawnSelectNode(TargetGraph, NodeSpec);
					if (!NewNode)
					{
						Result.Warnings.Add(FString::Printf(
							TEXT("Failed to spawn K2Node_Select for node '%s'."),
							*NodeSpec.Id));
					}
				}
				else if (NodeSpec.NodeType == FName(TEXT("K2Node_DynamicCast")))
				{
					NewNode = SpawnDynamicCastNode(TargetGraph, NodeSpec);
					if (!NewNode)
					{
						Result.Warnings.Add(FString::Printf(
							TEXT("Failed to spawn K2Node_DynamicCast for node '%s'."),
							*NodeSpec.Id));
					}
				}
				else
				{
					UE_LOG(LogSOTS_BlueprintGen, Warning,
						TEXT("ApplyGraphSpec: NodeType '%s' requires a spawner_key; skipping node '%s'."),
						*NodeSpec.NodeType.ToString(), *NodeSpec.Id);
					Result.Warnings.Add(FString::Printf(
						TEXT("NodeType '%s' not supported; provide a spawner_key for node '%s'."),
						*NodeSpec.NodeType.ToString(), *NodeSpec.Id));
				}
			}
		}

		if (NewNode)
		{
			if (bHasNodeId)
			{
				SetBPGenNodeId(NewNode, NodeSpec.NodeId);
				if (!bReuseExistingNode)
				{
					Result.CreatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}

			AddNodeToMap(NodeMap, NodeSpec.Id, NewNode);
		}
	}

	ApplyGraphLinks(Blueprint, TargetGraph, GraphSpec, NodeMap, SkippedSpecIds, Result);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ApplyGraphSpec: Failed to save Blueprint after applying graph."));
	}

	Result.ChangeSummary.BlueprintAssetPath = Target.BlueprintAssetPath;
	Result.ChangeSummary.TargetType = Target.TargetType;
	Result.ChangeSummary.TargetName = Target.Name;
	Result.ChangeSummary.CreatedNodeIds = Result.CreatedNodeIds;
	Result.ChangeSummary.UpdatedNodeIds = Result.UpdatedNodeIds;

	Result.bSuccess = true;
	return Result;
}

static FSOTS_BPGenApplyResult ApplyGraphSpecToFunction_Legacy(
	const UObject* WorldContextObject,
	const FSOTS_BPGenFunctionDef& FunctionDef,
	const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult CanonicalResult;
	FSOTS_BPGenGraphSpec CanonicalSpec = CanonicalizeGraphSpecForApply(GraphSpec, CanonicalResult);
	FSOTS_BPGenGraphTarget Target = CanonicalSpec.Target;
	Target.BlueprintAssetPath = Target.BlueprintAssetPath.IsEmpty() ? FunctionDef.TargetBlueprintPath : Target.BlueprintAssetPath;
	Target.TargetType = Target.TargetType.IsEmpty() ? TEXT("Function") : Target.TargetType;
	Target.Name = Target.Name.IsEmpty() ? FunctionDef.FunctionName.ToString() : Target.Name;
	Target.bCreateIfMissing = Target.bCreateIfMissing;

	FSOTS_BPGenApplyResult ApplyResult = ApplyGraphSpecInternal(WorldContextObject, Target, CanonicalSpec);
	ApplyResult.bSpecMigrated = CanonicalResult.bSpecMigrated;
	ApplyResult.MigrationNotes = CanonicalResult.MigrationNotes;
	return ApplyResult;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::ApplyGraphSpecToTarget(const UObject* WorldContextObject, const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult CanonicalResult;
	FSOTS_BPGenGraphSpec CanonicalSpec = CanonicalizeGraphSpecForApply(GraphSpec, CanonicalResult);
	FSOTS_BPGenGraphTarget Target = CanonicalSpec.Target;
	if (Target.TargetType.IsEmpty())
	{
		Target.TargetType = TEXT("Function");
	}

	FSOTS_BPGenApplyResult ApplyResult = ApplyGraphSpecInternal(WorldContextObject, Target, CanonicalSpec);
	ApplyResult.bSpecMigrated = CanonicalResult.bSpecMigrated;
	ApplyResult.MigrationNotes = CanonicalResult.MigrationNotes;
	return ApplyResult;
}

FSOTS_BPGenCanonicalizeResult USOTS_BPGenBuilder::CanonicalizeGraphSpec(const FSOTS_BPGenGraphSpec& GraphSpec, const FSOTS_BPGenSpecCanonicalizeOptions& Options)
{
	FSOTS_BPGenCanonicalizeResult Result;
	Result.CanonicalSpec = GraphSpec;
	CanonicalizeGraphSpecInternal(Result.CanonicalSpec, Options, Result.DiffNotes, Result.MigrationNotes, Result.bSpecMigrated);
	return Result;
}
		if (bVariableExists)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Variable '%s' already exists on blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *Blueprint->GetPathName());
			return;
		}

		const FSOTS_BPGenPin PinDef = BuildPinDefFromNodeSpec(NodeSpec);
		if (PinDef.Category.IsNone())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Node '%s' does not specify a PinCategory."),
				*NodeSpec.Id);
			return;
		}

		FEdGraphPinType PinType;
		if (!FillPinTypeFromBPGen(PinDef, PinType))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Failed to build pin type for variable '%s'."),
				*NodeSpec.VariableName.ToString());
			return;
		}

		Blueprint->Modify();
		UE_LOG(LogSOTS_BlueprintGen, Verbose,
			TEXT("EnsureBlueprintVariableFromNodeSpec: Creating variable '%s' (Category=%s, Default='%s')."),
			*NodeSpec.VariableName.ToString(),
			*PinDef.Category.ToString(),
			*PinDef.DefaultValue);

		if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, NodeSpec.VariableName, PinType, PinDef.DefaultValue))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Could not add variable '%s' to blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *Blueprint->GetPathName());
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Created variable '%s' (Category=%s, Default='%s') on blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *ResolvePinCategoryString(PinDef.Category), *PinDef.DefaultValue,
				*Blueprint->GetPathName());
		}
	}

	static bool SaveBlueprint(UBlueprint* Blueprint)
	{
		if (!Blueprint)
		{
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bWarnOfLongFilename = false;

		if (!UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs))
		{
			return false;
		}

		return true;
	}

	static FString NormalizeTargetType(const FString& InType)
	{
		FString Upper = InType;
		Upper.TrimStartAndEndInline();
		Upper = Upper.Replace(TEXT(" "), TEXT(""));
		Upper = Upper.ToUpper();
		return Upper.IsEmpty() ? TEXT("FUNCTION") : Upper;
	}

	static bool ResolveGraphForApply(const FSOTS_BPGenGraphTarget& Target, UBlueprint*& OutBlueprint, UEdGraph*& OutGraph, FSOTS_BPGenApplyResult& Result)
	{
		FString ResolveError;
		FString ResolveErrorCode;
		if (!USOTS_BPGenGraphResolver::ResolveTargetGraph(OutBlueprint, OutGraph, Target, ResolveError, ResolveErrorCode))
		{
			Result.ErrorCode = ResolveErrorCode;
			Result.ErrorMessage = ResolveError;
			Result.bSuccess = false;
			return false;
		}

		return true;
	}

	static UEdGraphNode* SpawnNodeFromSpawnerKey(UBlueprint* Blueprint, UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, FSOTS_BPGenApplyResult& Result)
	{
		if (!Graph)
		{
			Result.Warnings.Add(TEXT("SpawnNodeFromSpawnerKey: Graph is null."));
			return nullptr;
		}

		FSOTS_BPGenResolvedSpawner Resolved;
		const FName NodeType = NodeSpec.NodeType;

		auto TryResolveVariableSpawner = [&](bool bWantSetter) -> bool
		{
			FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
			const FBlueprintActionDatabase::FActionRegistry& Registry = ActionDB.GetAllActions();

			for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Entry : Registry)
			{
				for (UBlueprintNodeSpawner* Spawner : Entry.Value)
				{
					UBlueprintVariableNodeSpawner* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(Spawner);
					if (!VarSpawner)
					{
						continue;
					}

					if (VarSpawner->IsPropertySetter() != bWantSetter)
					{
						continue;
					}

					UStruct* OwnerStruct = VarSpawner->GetVarOuter();
					if (!OwnerStruct)
					{
						continue;
					}

					const FString Key = FString::Printf(TEXT("%s:%s"), *OwnerStruct->GetPathName(), *VarSpawner->GetVarName().ToString());
					if (Key.Equals(NodeSpec.SpawnerKey, ESearchCase::CaseSensitive))
					{
						Resolved.SpawnerKey = NodeSpec.SpawnerKey;
						Resolved.Spawner = VarSpawner;
						Resolved.DebugName = VarSpawner->GetVarName().ToString();
						Resolved.DebugCategory = TEXT("VariableSpawner");
						return true;
					}
				}
			}

			return false;
		};

		if (NodeType == FName(TEXT("K2Node_VariableSet")) || NodeType.ToString().Equals(TEXT("variable_set"), ESearchCase::IgnoreCase))
		{
			TryResolveVariableSpawner(true);
		}
		else if (NodeType == FName(TEXT("K2Node_VariableGet")) || NodeType.ToString().Equals(TEXT("variable_get"), ESearchCase::IgnoreCase))
		{
			TryResolveVariableSpawner(false);
		}

		FString ResolveError;
		if (!Resolved.Spawner.IsValid() && !FSOTS_BPGenSpawnerRegistry::ResolveForContext(Blueprint, Graph, NodeSpec.SpawnerKey, Resolved, ResolveError))
		{
			Result.Warnings.Add(FString::Printf(TEXT("SpawnNodeFromSpawnerKey: %s"), *ResolveError));
			AddErrorCode(Result, TEXT("ERR_SPAWNER_MISSING"));
			return nullptr;
		}

		UBlueprintNodeSpawner* Spawner = Resolved.Spawner.Get();
		if (!Spawner)
		{
			Result.Warnings.Add(FString::Printf(TEXT("SpawnNodeFromSpawnerKey: Spawner '%s' was invalid."), *Resolved.SpawnerKey));
			AddErrorCode(Result, TEXT("ERR_SPAWNER_MISSING"));
			return nullptr;
		}

		Graph->Modify();

		UEdGraphNode* NewNode = Spawner->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), NodeSpec.NodePosition);
		if (!NewNode)
		{
			Result.Warnings.Add(FString::Printf(TEXT("SpawnNodeFromSpawnerKey: Failed to invoke spawner '%s'."), *Resolved.SpawnerKey));
			return nullptr;
		}

		NewNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		NewNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
		ApplyExtraPinDefaults(NewNode, NodeSpec);
		return NewNode;
	}

	static UEdGraphNode* SpawnDynamicCastNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		FString TargetTypePath;
		if (const FString* TargetTypeExtra = NodeSpec.ExtraData.Find(FName(TEXT("TargetType"))))
		{
			TargetTypePath = *TargetTypeExtra;
		}
		else if (!NodeSpec.StructPath.IsEmpty())
		{
			TargetTypePath = NodeSpec.StructPath;
		}

		if (TargetTypePath.IsEmpty())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnDynamicCastNode: Node '%s' missing TargetType/StructPath."), *NodeSpec.Id);
			return nullptr;
		}

		UClass* TargetClass = FindObject<UClass>(nullptr, *TargetTypePath);
		if (!TargetClass)
		{
			TargetClass = LoadObject<UClass>(nullptr, *TargetTypePath);
		}

		if (!TargetClass)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnDynamicCastNode: Could not load TargetClass '%s' for node '%s'."),
				*TargetTypePath, *NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
		Graph->AddNode(CastNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		CastNode->SetFlags(RF_Transactional);
		CastNode->TargetType = TargetClass;
		CastNode->SetPurity(false);
		CastNode->CreateNewGuid();
		CastNode->PostPlacedNewNode();
		CastNode->AllocateDefaultPins();

		CastNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		CastNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(CastNode, NodeSpec);

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SpawnDynamicCastNode: Node '%s' pins after creation (TargetType=%s):"),
			*NodeSpec.Id, *TargetTypePath);
		for (UEdGraphPin* Pin : CastNode->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		return CastNode;
	}

	static UEdGraphNode* SpawnKnotNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_Knot* KnotNode = NewObject<UK2Node_Knot>(Graph);
		Graph->AddNode(KnotNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		KnotNode->SetFlags(RF_Transactional);
		KnotNode->CreateNewGuid();
		KnotNode->PostPlacedNewNode();
		KnotNode->AllocateDefaultPins();

		KnotNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		KnotNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(KnotNode, NodeSpec);

		return KnotNode;
	}

	static void ApplyExtraPinDefaults(UEdGraphNode* Node, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Node)
		{
			return;
		}

		TMap<FString, FSOTS_BPGenPin> PinTypeOverrides;

		for (const TPair<FName, FString>& ExtraPair : NodeSpec.ExtraData)
		{
			const FString KeyString = ExtraPair.Key.ToString();
			FString PinName;
			FString Attribute;
			if (KeyString.Split(TEXT("."), &PinName, &Attribute))
			{
				FSOTS_BPGenPin& Override = PinTypeOverrides.FindOrAdd(PinName);
				if (Attribute.Equals(TEXT("PinCategory"), ESearchCase::IgnoreCase))
				{
					Override.Category = FName(*ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("PinSubCategory"), ESearchCase::IgnoreCase))
				{
					Override.SubCategory = FName(*ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("SubObjectPath"), ESearchCase::IgnoreCase))
				{
					Override.SubObjectPath = ExtraPair.Value;
				}
				else if (Attribute.Equals(TEXT("ContainerType"), ESearchCase::IgnoreCase))
				{
					Override.ContainerType = ParseContainerType(ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("DefaultValue"), ESearchCase::IgnoreCase))
				{
					Override.DefaultValue = ExtraPair.Value;
				}
				continue;
			}

			if (UEdGraphPin* Pin = FindPinByName(Node, ExtraPair.Key))
			{
				const bool bIsClassPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class;
				if (!bIsClassPin)
				{
					Pin->DefaultValue = ExtraPair.Value;
				}

				TryAssignDefaultObject(Pin, ExtraPair.Value);
			}
		}

		for (const TPair<FString, FSOTS_BPGenPin>& OverridePair : PinTypeOverrides)
		{
			if (UEdGraphPin* Pin = FindPinByName(Node, FName(*OverridePair.Key)))
			{
				FEdGraphPinType NewType;
				if (FillPinTypeFromBPGen(OverridePair.Value, NewType))
				{
					Pin->PinType = NewType;
				}

				if (!OverridePair.Value.DefaultValue.IsEmpty())
				{
					const bool bIsClassPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class;
					if (!bIsClassPin)
					{
						Pin->DefaultValue = OverridePair.Value.DefaultValue;
					}

					TryAssignDefaultObject(Pin, OverridePair.Value.DefaultValue);
				}
			}
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			const bool bIsClassOrObjectPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object;
			if (bIsClassOrObjectPin && !Pin->DefaultValue.IsEmpty())
			{
				TryAssignDefaultObject(Pin, Pin->DefaultValue);
			}

			const bool bIsTransformPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
				&& Pin->PinType.PinSubCategoryObject == TBaseStructure<FTransform>::Get();
			if (bIsTransformPin && !Pin->DefaultValue.IsEmpty())
			{
				FTransform ParsedTransform = FTransform::Identity;
				if (ParsedTransform.InitFromString(Pin->DefaultValue))
				{
					Pin->DefaultValue = ParsedTransform.ToString();
				}
				else
				{
					Pin->DefaultValue = FTransform::Identity.ToString();
				}
			}
		}
	}

	static void ApplySelectOptionDefaults(UK2Node_Select* SelectNode, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!SelectNode)
		{
			return;
		}

		TArray<UEdGraphPin*> OptionPins;
		SelectNode->GetOptionPins(OptionPins);

		for (int32 Index = 0; Index < OptionPins.Num(); ++Index)
		{
			UEdGraphPin* OptionPin = OptionPins[Index];
			if (!OptionPin)
			{
				continue;
			}

			const FString Key = FString::Printf(TEXT("Option %d.DefaultValue"), Index);
			if (const FString* DefaultPtr = NodeSpec.ExtraData.Find(*Key))
			{
				OptionPin->DefaultValue = *DefaultPtr;
			}
		}

		SelectNode->NodeConnectionListChanged();
	}

	static void ClearNonExecPins(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return;
		}

		Node->Modify();

		TArray<UEdGraphPin*> PinsToRemove;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			PinsToRemove.Add(Pin);
		}

	for (UEdGraphPin* Pin : PinsToRemove)
	{
		Pin->BreakAllPinLinks();
		Node->RemovePin(Pin);
	}
}

static UEdGraphPin* FindPinByName(UEdGraphNode* Node, FName PinName)
{
	if (!Node)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName == PinName)
		{
			return Pin;
		}
	}

	const FString Requested = PinName.ToString();
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName.ToString().Equals(Requested, ESearchCase::IgnoreCase))
		{
			return Pin;
		}
	}

	return nullptr;
}

static UEdGraphPin* FindPinHeuristic(UEdGraphNode* Node, FName PinName)
{
	if (!Node)
	{
		return nullptr;
	}

	const FString Requested = PinName.ToString();
	const FString RequestedLower = Requested.ToLower();

	// Exact (case-insensitive) match first.
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName.ToString().Equals(Requested, ESearchCase::IgnoreCase))
		{
			return Pin;
		}
	}

	static const TMap<FString, TArray<FString>> PinAliases = {
		{TEXT("exec"), {TEXT("execute"), TEXT("then"), TEXT("output")}},
		{TEXT("execute"), {TEXT("exec"), TEXT("then"), TEXT("input")}},
		{TEXT("then"), {TEXT("exec"), TEXT("execute"), TEXT("output")}},
		{TEXT("return"), {TEXT("ReturnValue"), TEXT("Return Value"), TEXT("output")}},
		{TEXT("returnvalue"), {TEXT("return"), TEXT("Return Value"), TEXT("output")}},
		{TEXT("target"), {TEXT("Target"), TEXT("self"), TEXT("Self")}},
		{TEXT("instring"), {TEXT("string"), TEXT("text")}},
		{TEXT("string"), {TEXT("instring"), TEXT("text")}},
		{TEXT("intext"), {TEXT("text"), TEXT("string")}},
		{TEXT("text"), {TEXT("intext"), TEXT("string")}}
	};

	if (const TArray<FString>* Aliases = PinAliases.Find(RequestedLower))
	{
		for (const FString& Alias : *Aliases)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->PinName.ToString().Equals(Alias, ESearchCase::IgnoreCase))
				{
					return Pin;
				}
			}
		}
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin)
		{
			continue;
		}

		const FString Candidate = Pin->PinName.ToString();
		if (Candidate.Contains(Requested, ESearchCase::IgnoreCase) || Requested.Contains(Candidate, ESearchCase::IgnoreCase))
		{
			return Pin;
		}
	}

	return nullptr;
}

struct FSOTS_BPGenAutoFixState
{
	bool bEnabled = false;
	bool bInsertConversions = false;
	int32 MaxSteps = 0;
	int32 StepIndex = 0;
};

static bool CanAutoFix(const FSOTS_BPGenAutoFixState& State)
{
	return State.bEnabled && State.StepIndex < State.MaxSteps;
}

static void AddErrorCode(FSOTS_BPGenApplyResult& Result, const FString& Code)
{
	Result.ErrorCodes.AddUnique(Code);
}

static bool RecordAutoFixStep(FSOTS_BPGenAutoFixState& State, FSOTS_BPGenApplyResult& Result, const FString& Code, const FString& Description, const TArray<FString>& NodeIds, const TArray<FString>& Pins, const FString& Before, const FString& After)
{
	if (!CanAutoFix(State))
	{
		return false;
	}

	FSOTS_BPGenAutoFixStep Step;
	Step.StepIndex = State.StepIndex++;
	Step.Code = Code;
	Step.Description = Description;
	Step.AffectedNodeIds = NodeIds;
	Step.AffectedPins = Pins;
	Step.Before = Before;
	Step.After = After;
	Result.AutoFixSteps.Add(Step);
	return true;
}

static FString DescribePinType(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return TEXT("None");
	}

	const FEdGraphPinType& Type = Pin->PinType;
	FString Description = Type.PinCategory.ToString();
	if (Type.PinSubCategory != NAME_None)
	{
		Description += FString::Printf(TEXT("(%s)"), *Type.PinSubCategory.ToString());
	}
	if (Type.PinSubCategoryObject.IsValid())
	{
		Description += FString::Printf(TEXT(":%s"), *Type.PinSubCategoryObject->GetPathName());
	}
	if (Type.ContainerType != EPinContainerType::None)
	{
		Description += FString::Printf(TEXT("[%s]"), *UEnum::GetValueAsString(Type.ContainerType));
	}
	return Description;
}

static FString FormatPinId(const FString& NodeId, const FName& PinName)
{
	return FString::Printf(TEXT("%s.%s"), *NodeId, *PinName.ToString());
}

struct FAutoFixConversionRule
{
	FName FromCategory;
	FName ToCategory;
	FString FunctionPath;
	FName InputPinName;
	FName OutputPinName;
	FString Label;
};

static bool FindConversionRule(const FEdGraphPinType& FromType, const FEdGraphPinType& ToType, FAutoFixConversionRule& OutRule)
{
	if (FromType.ContainerType != EPinContainerType::None || ToType.ContainerType != EPinContainerType::None)
	{
		return false;
	}

	const FName FromCategory = FromType.PinCategory;
	const FName ToCategory = ToType.PinCategory;
	if (FromCategory == UEdGraphSchema_K2::PC_Exec || ToCategory == UEdGraphSchema_K2::PC_Exec)
	{
		return false;
	}

	auto Match = [&](FName InFrom, FName InTo, const TCHAR* FunctionPath, const TCHAR* InputName, const TCHAR* Label)
	{
		if (FromCategory == InFrom && ToCategory == InTo)
		{
			OutRule.FromCategory = InFrom;
			OutRule.ToCategory = InTo;
			OutRule.FunctionPath = FunctionPath;
			OutRule.InputPinName = FName(InputName);
			OutRule.OutputPinName = UEdGraphSchema_K2::PN_ReturnValue;
			OutRule.Label = Label;
			return true;
		}
		return false;
	};

	if (Match(UEdGraphSchema_K2::PC_Boolean, UEdGraphSchema_K2::PC_Int, TEXT("/Script/Engine.KismetMathLibrary:Conv_BoolToInt"), TEXT("InBool"), TEXT("BoolToInt")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_Int, UEdGraphSchema_K2::PC_Boolean, TEXT("/Script/Engine.KismetMathLibrary:Conv_IntToBool"), TEXT("InInt"), TEXT("IntToBool")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_Int, UEdGraphSchema_K2::PC_Float, TEXT("/Script/Engine.KismetMathLibrary:Conv_IntToFloat"), TEXT("InInt"), TEXT("IntToFloat")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_Float, UEdGraphSchema_K2::PC_Int, TEXT("/Script/Engine.KismetMathLibrary:Conv_FloatToInt"), TEXT("InFloat"), TEXT("FloatToInt")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_Name, UEdGraphSchema_K2::PC_String, TEXT("/Script/Engine.KismetStringLibrary:Conv_NameToString"), TEXT("InName"), TEXT("NameToString")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_String, UEdGraphSchema_K2::PC_Name, TEXT("/Script/Engine.KismetStringLibrary:Conv_StringToName"), TEXT("InString"), TEXT("StringToName")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_String, UEdGraphSchema_K2::PC_Text, TEXT("/Script/Engine.KismetTextLibrary:Conv_StringToText"), TEXT("InString"), TEXT("StringToText")))
	{
		return true;
	}
	if (Match(UEdGraphSchema_K2::PC_Text, UEdGraphSchema_K2::PC_String, TEXT("/Script/Engine.KismetTextLibrary:Conv_TextToString"), TEXT("InText"), TEXT("TextToString")))
	{
		return true;
	}

	return false;
}

static bool IsSkippableConversionInputPin(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return true;
	}

	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
	{
		return true;
	}

	const FString PinName = Pin->PinName.ToString();
	return PinName.Equals(UEdGraphSchema_K2::PN_Self.ToString(), ESearchCase::IgnoreCase)
		|| PinName.Equals(TEXT("WorldContextObject"), ESearchCase::IgnoreCase);
}

static UEdGraphPin* FindConversionInputPin(UEdGraphNode* Node, const FAutoFixConversionRule& Rule)
{
	if (!Node)
	{
		return nullptr;
	}

	if (UEdGraphPin* Preferred = FindPinByName(Node, Rule.InputPinName))
	{
		return Preferred;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->Direction != EGPD_Input)
		{
			continue;
		}

		if (IsSkippableConversionInputPin(Pin))
		{
			continue;
		}

		return Pin;
	}

	return nullptr;
}

static UEdGraphPin* FindConversionOutputPin(UEdGraphNode* Node, const FAutoFixConversionRule& Rule)
{
	if (!Node)
	{
		return nullptr;
	}

	if (UEdGraphPin* Preferred = FindPinByName(Node, Rule.OutputPinName))
	{
		return Preferred;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->Direction != EGPD_Output)
		{
			continue;
		}

		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
		{
			continue;
		}

		return Pin;
	}

	return nullptr;
}

	static bool ValidateLinkPins(const FSOTS_BPGenGraphLink& Link, UEdGraphPin* FromPin, UEdGraphPin* ToPin, FSOTS_BPGenApplyResult& Result)
	{
		if (!FromPin || !ToPin)
		{
			return false;
		}

		bool bIsValid = true;
		if (FromPin->Direction != EGPD_Output)
		{
			const FString Message = FString::Printf(
				TEXT("Link '%s.%s' expects an output pin but '%s' has direction '%s'."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.FromNodeId,
				*GetPinDirectionText(FromPin->Direction));
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("%s"), *Message);
			Result.Warnings.Add(Message);
			bIsValid = false;
		}

		if (ToPin->Direction != EGPD_Input)
		{
			const FString Message = FString::Printf(
				TEXT("Link '%s.%s' expects an input pin but '%s' has direction '%s'."),
				*Link.ToNodeId,
				*Link.ToPinName.ToString(),
				*Link.ToNodeId,
				*GetPinDirectionText(ToPin->Direction));
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("%s"), *Message);
			Result.Warnings.Add(Message);
			bIsValid = false;
		}

		return bIsValid;
	}

	static bool ConnectPinsSchemaFirst(const FSOTS_BPGenGraphLink& Link, UEdGraphPin* FromPin, UEdGraphPin* ToPin, FSOTS_BPGenApplyResult& Result)
	{
		if (!FromPin || !ToPin)
		{
			Result.Warnings.Add(TEXT("ConnectPinsSchemaFirst: Null pin."));
			return false;
		}

		if (Link.bBreakExistingFrom)
		{
			FromPin->BreakAllPinLinks(true);
		}

		if (Link.bBreakExistingTo)
		{
			ToPin->BreakAllPinLinks(true);
		}

		const UEdGraphSchema* Schema = FromPin->GetSchema();
		if (!Schema)
		{
			Schema = ToPin->GetSchema();
		}

		bool bSchemaAttempted = false;
		if (Link.bUseSchema && Schema)
		{
			bSchemaAttempted = true;
			if (Schema->TryCreateConnection(FromPin, ToPin))
			{
				return true;
			}

			const FString Warning = FString::Printf(
				TEXT("Schema rejected connection %s.%s -> %s.%s."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString());
			Result.Warnings.Add(Warning);
		}

		const bool bAllowFallback = CVarSOTS_BPGenAllowMakeLinkFallback.GetValueOnAnyThread() != 0;
		if (!bAllowFallback)
		{
			Result.Warnings.Add(FString::Printf(
				TEXT("Connection %s.%s -> %s.%s not created: MakeLinkTo fallback disabled."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString()));
			return false;
		}

		FromPin->MakeLinkTo(ToPin);

		if (UEdGraphNode* OwningNode = FromPin->GetOwningNode())
		{
			OwningNode->NodeConnectionListChanged();
		}
		if (UEdGraphNode* TargetOwningNode = ToPin->GetOwningNode())
		{
			TargetOwningNode->NodeConnectionListChanged();
		}

	if (!bSchemaAttempted && !Link.bUseSchema)
	{
		Result.Warnings.Add(FString::Printf(
			TEXT("Connection %s.%s -> %s.%s used MakeLinkTo (schema disabled)."),
			*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString()));
		}

	return true;
}

static bool TryInsertConversionNode(
	UBlueprint* Blueprint,
	UEdGraph* Graph,
	const FSOTS_BPGenGraphLink& Link,
	const FString& FromNodeId,
	const FString& ToNodeId,
	UEdGraphPin* FromPin,
	UEdGraphPin* ToPin,
	const FAutoFixConversionRule& Rule,
	FSOTS_BPGenAutoFixState& AutoFixState,
	FSOTS_BPGenApplyResult& Result)
{
	if (!Graph || !Blueprint || !FromPin || !ToPin)
	{
		return false;
	}

	if (!CanAutoFix(AutoFixState))
	{
		return false;
	}

	const FString ConversionNodeId = FString::Printf(TEXT("%s__to__%s__conv__%s"), *FromNodeId, *ToNodeId, *Rule.Label);
	UEdGraphNode* ConversionNode = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node->NodeComment == ConversionNodeId)
		{
			ConversionNode = Node;
			break;
		}
	}

	const FVector2D FromPos(FromPin->GetOwningNode()->NodePosX, FromPin->GetOwningNode()->NodePosY);
	const FVector2D ToPos(ToPin->GetOwningNode()->NodePosX, ToPin->GetOwningNode()->NodePosY);
	const FVector2D ConvPos((FromPos + ToPos) * 0.5f + FVector2D(60.f, 40.f));

	bool bCreated = false;
	if (!ConversionNode)
	{
		FSOTS_BPGenGraphNode NodeSpec;
		NodeSpec.Id = ConversionNodeId;
		NodeSpec.NodeId = ConversionNodeId;
		NodeSpec.NodeType = FName(TEXT("K2Node_CallFunction"));
		NodeSpec.SpawnerKey = Rule.FunctionPath;
		NodeSpec.bPreferSpawnerKey = true;
		NodeSpec.NodePosition = ConvPos;

		ConversionNode = SpawnNodeFromSpawnerKey(Blueprint, Graph, NodeSpec, Result);
		if (!ConversionNode)
		{
			Result.Warnings.Add(FString::Printf(TEXT("AutoFix: Failed to spawn conversion node '%s'."), *ConversionNodeId));
			return false;
		}

		ConversionNode->NodeComment = ConversionNodeId;
		Result.CreatedNodeIds.AddUnique(ConversionNodeId);
		bCreated = true;
	}

	UEdGraphPin* ConvInput = FindConversionInputPin(ConversionNode, Rule);
	UEdGraphPin* ConvOutput = FindConversionOutputPin(ConversionNode, Rule);
	if (!ConvInput || !ConvOutput)
	{
		Result.Warnings.Add(FString::Printf(TEXT("AutoFix: Conversion node '%s' is missing expected pins."), *ConversionNodeId));
		return false;
	}

	FSOTS_BPGenGraphLink LinkToConv;
	LinkToConv.FromNodeId = FromNodeId;
	LinkToConv.FromPinName = FromPin->PinName;
	LinkToConv.ToNodeId = ConversionNodeId;
	LinkToConv.ToPinName = ConvInput->PinName;
	LinkToConv.bBreakExistingFrom = Link.bBreakExistingFrom;
	LinkToConv.bBreakExistingTo = false;
	LinkToConv.bUseSchema = true;

	FSOTS_BPGenGraphLink LinkFromConv;
	LinkFromConv.FromNodeId = ConversionNodeId;
	LinkFromConv.FromPinName = ConvOutput->PinName;
	LinkFromConv.ToNodeId = ToNodeId;
	LinkFromConv.ToPinName = ToPin->PinName;
	LinkFromConv.bBreakExistingFrom = false;
	LinkFromConv.bBreakExistingTo = Link.bBreakExistingTo;
	LinkFromConv.bUseSchema = true;

	const bool bFirstOk = ConnectPinsSchemaFirst(LinkToConv, FromPin, ConvInput, Result);
	const bool bSecondOk = ConnectPinsSchemaFirst(LinkFromConv, ConvOutput, ToPin, Result);
	if (!bFirstOk || !bSecondOk)
	{
		Result.Warnings.Add(FString::Printf(TEXT("AutoFix: Failed to wire conversion node '%s' into the connection."), *ConversionNodeId));
		return false;
	}

	const FString Before = FString::Printf(TEXT("%s.%s -> %s.%s"), *FromNodeId, *FromPin->PinName.ToString(), *ToNodeId, *ToPin->PinName.ToString());
	const FString After = FString::Printf(TEXT("%s.%s -> %s.%s -> %s.%s"),
		*FromNodeId,
		*FromPin->PinName.ToString(),
		*ConversionNodeId,
		*ConvInput->PinName.ToString(),
		*ToNodeId,
		*ToPin->PinName.ToString());

	TArray<FString> PinIds;
	PinIds.Add(FormatPinId(FromNodeId, FromPin->PinName));
	PinIds.Add(FormatPinId(ConversionNodeId, ConvInput->PinName));
	PinIds.Add(FormatPinId(ConversionNodeId, ConvOutput->PinName));
	PinIds.Add(FormatPinId(ToNodeId, ToPin->PinName));

	TArray<FString> NodeIds;
	NodeIds.Add(FromNodeId);
	NodeIds.Add(ConversionNodeId);
	NodeIds.Add(ToNodeId);

	RecordAutoFixStep(
		AutoFixState,
		Result,
		TEXT("FIX_INSERT_CONVERSION"),
		FString::Printf(TEXT("Inserted conversion node '%s' (%s)."), *ConversionNodeId, *Rule.FunctionPath),
		NodeIds,
		PinIds,
		Before,
		After);

	Result.Warnings.Add(FString::Printf(TEXT("AutoFix inserted conversion node '%s' for %s -> %s."), *ConversionNodeId, *FromPin->PinType.PinCategory.ToString(), *ToPin->PinType.PinCategory.ToString()));
	if (!bCreated)
	{
		Result.UpdatedNodeIds.AddUnique(ConversionNodeId);
	}

	return true;
}

static bool ConnectPinsWithAutoFix(
	UBlueprint* Blueprint,
	UEdGraph* Graph,
	const FSOTS_BPGenGraphLink& Link,
	const FString& FromNodeId,
	const FString& ToNodeId,
	UEdGraphPin* FromPin,
	UEdGraphPin* ToPin,
	FSOTS_BPGenAutoFixState& AutoFixState,
	FSOTS_BPGenApplyResult& Result)
{
	if (!FromPin || !ToPin)
	{
		Result.Warnings.Add(TEXT("ConnectPinsWithAutoFix: Null pin."));
		return false;
	}

	if (Link.bBreakExistingFrom)
	{
		FromPin->BreakAllPinLinks(true);
	}

	if (Link.bBreakExistingTo)
	{
		ToPin->BreakAllPinLinks(true);
	}

	const UEdGraphSchema* Schema = FromPin->GetSchema();
	if (!Schema)
	{
		Schema = ToPin->GetSchema();
	}

	FPinConnectionResponse Response;
	bool bHasResponse = false;
	if (Schema)
	{
		Response = Schema->CanCreateConnection(FromPin, ToPin);
		bHasResponse = true;
	}

	if (Link.bUseSchema && Schema)
	{
		if (Schema->TryCreateConnection(FromPin, ToPin))
		{
			return true;
		}
	}

	if (Link.bUseSchema && Schema)
	{
		const FString ResponseMessage = bHasResponse ? Response.Message.ToString() : FString();
		FAutoFixConversionRule Rule;
		const bool bHasConversion = FindConversionRule(FromPin->PinType, ToPin->PinType, Rule);

		if (bHasConversion)
		{
			if (AutoFixState.bEnabled && AutoFixState.bInsertConversions)
			{
				if (TryInsertConversionNode(Blueprint, Graph, Link, FromNodeId, ToNodeId, FromPin, ToPin, Rule, AutoFixState, Result))
				{
					return true;
				}
			}
			else
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Schema rejected connection %s.%s -> %s.%s (from=%s, to=%s). Suggested conversion: %s (enable auto_fix_insert_conversions)."),
					*FromNodeId,
					*Link.FromPinName.ToString(),
					*ToNodeId,
					*Link.ToPinName.ToString(),
					*DescribePinType(FromPin),
					*DescribePinType(ToPin),
					*Rule.FunctionPath));
			}
		}

		const bool bTypeMismatch = FromPin->PinType.PinCategory != ToPin->PinType.PinCategory
			|| FromPin->PinType.PinSubCategory != ToPin->PinType.PinSubCategory
			|| FromPin->PinType.PinSubCategoryObject != ToPin->PinType.PinSubCategoryObject
			|| FromPin->PinType.ContainerType != ToPin->PinType.ContainerType;

		AddErrorCode(Result, bTypeMismatch ? TEXT("ERR_SCHEMA_REJECTED_TYPE_MISMATCH") : TEXT("ERR_SCHEMA_REJECTED"));

		FString Warning = FString::Printf(
			TEXT("Schema rejected connection %s.%s -> %s.%s (from=%s, to=%s)."),
			*FromNodeId,
			*Link.FromPinName.ToString(),
			*ToNodeId,
			*Link.ToPinName.ToString(),
			*DescribePinType(FromPin),
			*DescribePinType(ToPin));

		if (!ResponseMessage.IsEmpty())
		{
			Warning += FString::Printf(TEXT(" Reason: %s"), *ResponseMessage);
		}

		Result.Warnings.Add(Warning);
	}

	const bool bAllowFallback = CVarSOTS_BPGenAllowMakeLinkFallback.GetValueOnAnyThread() != 0;
	if (!bAllowFallback)
	{
		Result.Warnings.Add(FString::Printf(
			TEXT("Connection %s.%s -> %s.%s not created: MakeLinkTo fallback disabled."),
			*FromNodeId,
			*Link.FromPinName.ToString(),
			*ToNodeId,
			*Link.ToPinName.ToString()));
		return false;
	}

	FromPin->MakeLinkTo(ToPin);

	if (UEdGraphNode* OwningNode = FromPin->GetOwningNode())
	{
		OwningNode->NodeConnectionListChanged();
	}
	if (UEdGraphNode* TargetOwningNode = ToPin->GetOwningNode())
	{
		TargetOwningNode->NodeConnectionListChanged();
	}

	if (!Link.bUseSchema)
	{
		Result.Warnings.Add(FString::Printf(
			TEXT("Connection %s.%s -> %s.%s used MakeLinkTo (schema disabled)."),
			*FromNodeId,
			*Link.FromPinName.ToString(),
			*ToNodeId,
			*Link.ToPinName.ToString()));
	}

	return true;
}

static void ApplyGraphLinks(UBlueprint* Blueprint, UEdGraph* Graph, const FSOTS_BPGenGraphSpec& GraphSpec, const TMap<FString, UEdGraphNode*>& NodeMap, const TSet<FString>& SkippedSpecIds, FSOTS_BPGenApplyResult& Result)
{
	FSOTS_BPGenAutoFixState AutoFixState;
	AutoFixState.bEnabled = GraphSpec.bAutoFix;
	AutoFixState.bInsertConversions = GraphSpec.bAutoFixInsertConversions;
	AutoFixState.MaxSteps = FMath::Max(0, GraphSpec.AutoFixMaxSteps);

	static const TMap<FString, TArray<FString>> PinAliasMap = {
		{TEXT("exec"), {TEXT("execute"), TEXT("then"), TEXT("output")}},
		{TEXT("execute"), {TEXT("exec"), TEXT("then"), TEXT("input")}},
		{TEXT("then"), {TEXT("exec"), TEXT("execute"), TEXT("output")}},
		{TEXT("return"), {TEXT("ReturnValue"), TEXT("Return Value"), TEXT("output")}},
		{TEXT("returnvalue"), {TEXT("return"), TEXT("Return Value"), TEXT("output")}},
		{TEXT("target"), {TEXT("Target"), TEXT("self"), TEXT("Self")}},
		{TEXT("instring"), {TEXT("string"), TEXT("text")}},
		{TEXT("string"), {TEXT("instring"), TEXT("text")}},
		{TEXT("intext"), {TEXT("text"), TEXT("string")}},
		{TEXT("text"), {TEXT("intext"), TEXT("string")}}
	};

	auto TryResolveAlias = [&](UEdGraphNode* Node, const FName& RequestedPinName, EEdGraphPinDirection Direction, FName& OutResolvedName, UEdGraphPin*& OutResolvedPin) -> bool
	{
		if (!Node)
		{
			return false;
		}

		const FString Requested = RequestedPinName.ToString();
		const FString RequestedLower = Requested.ToLower();
		TArray<UEdGraphPin*> Candidates;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			if (Direction != EGPD_MAX && Pin->Direction != Direction)
			{
				continue;
			}

			Candidates.Add(Pin);
		}

		TArray<UEdGraphPin*> Matches;
		for (UEdGraphPin* Pin : Candidates)
		{
			if (Pin && Pin->PinName.ToString().Equals(Requested, ESearchCase::IgnoreCase))
			{
				Matches.Add(Pin);
			}
		}

		auto TryAliasList = [&](const TArray<FString>& Aliases)
		{
			for (const FString& Alias : Aliases)
			{
				for (UEdGraphPin* Pin : Candidates)
				{
					if (Pin && Pin->PinName.ToString().Equals(Alias, ESearchCase::IgnoreCase))
					{
						Matches.Add(Pin);
					}
				}
			}
		};

		if (Matches.Num() == 0)
		{
			if (const TArray<FString>* Aliases = PinAliasMap.Find(RequestedLower))
			{
				TryAliasList(*Aliases);
			}
		}

		TSet<UEdGraphPin*> UniqueMatches;
		for (UEdGraphPin* Pin : Matches)
		{
			if (Pin)
			{
				UniqueMatches.Add(Pin);
			}
		}

		if (UniqueMatches.Num() == 1)
		{
			OutResolvedPin = *UniqueMatches.CreateConstIterator();
			OutResolvedName = OutResolvedPin->PinName;
			return true;
		}

		return false;
	};

	for (const FSOTS_BPGenGraphLink& Link : GraphSpec.Links)
	{
		if (SkippedSpecIds.Contains(Link.FromNodeId) || SkippedSpecIds.Contains(Link.ToNodeId))
		{
			Result.Warnings.Add(FString::Printf(
				TEXT("Skipped link %s.%s -> %s.%s because one or both nodes were skipped."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString()));
			continue;
		}

		UEdGraphNode* const* FromNodePtr = NodeMap.Find(Link.FromNodeId);
		UEdGraphNode* const* ToNodePtr = NodeMap.Find(Link.ToNodeId);

		if (!FromNodePtr || !ToNodePtr || !(*FromNodePtr) || !(*ToNodePtr))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("ApplyGraphSpec: Link from '%s' to '%s' could not find nodes."),
				*Link.FromNodeId, *Link.ToNodeId);
			Result.Warnings.Add(FString::Printf(
				TEXT("Link from '%s' to '%s' could not find nodes."),
				*Link.FromNodeId, *Link.ToNodeId));
			AddErrorCode(Result, TEXT("ERR_NODE_NOT_FOUND"));
			continue;
		}

		FString FromNodeId = Link.FromNodeId;
		FString ToNodeId = Link.ToNodeId;
		FName FromPinName = Link.FromPinName;
		FName ToPinName = Link.ToPinName;

		UEdGraphPin* FromPin = FindPinByName(*FromNodePtr, FromPinName);
		UEdGraphPin* ToPin = FindPinByName(*ToNodePtr, ToPinName);

		if (!FromPin && Link.bAllowHeuristicPinMatch)
		{
			FromPin = FindPinHeuristic(*FromNodePtr, FromPinName);
			if (FromPin)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Heuristic matched source pin '%s' on node '%s'."),
					*FromPinName.ToString(), *FromNodeId));
			}
		}

		if (!ToPin && Link.bAllowHeuristicPinMatch)
		{
			ToPin = FindPinHeuristic(*ToNodePtr, ToPinName);
			if (ToPin)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Heuristic matched target pin '%s' on node '%s'."),
					*ToPinName.ToString(), *ToNodeId));
			}
		}

		if (!FromPin && CanAutoFix(AutoFixState))
		{
			FName ResolvedName;
			UEdGraphPin* ResolvedPin = nullptr;
			if (TryResolveAlias(*FromNodePtr, FromPinName, EGPD_Output, ResolvedName, ResolvedPin))
			{
				const FString Before = FormatPinId(FromNodeId, FromPinName);
				const FString After = FormatPinId(FromNodeId, ResolvedName);
				FromPinName = ResolvedName;
				FromPin = ResolvedPin;

				TArray<FString> NodeIds;
				NodeIds.Add(FromNodeId);
				TArray<FString> PinIds;
				PinIds.Add(After);
				RecordAutoFixStep(
					AutoFixState,
					Result,
					TEXT("FIX_PIN_ALIAS"),
					FString::Printf(TEXT("Matched source pin alias on '%s'."), *FromNodeId),
					NodeIds,
					PinIds,
					Before,
					After);

				Result.Warnings.Add(FString::Printf(TEXT("Heuristic pin match applied: %s -> %s"), *Before, *After));
			}
		}

		if (!ToPin && CanAutoFix(AutoFixState))
		{
			FName ResolvedName;
			UEdGraphPin* ResolvedPin = nullptr;
			if (TryResolveAlias(*ToNodePtr, ToPinName, EGPD_Input, ResolvedName, ResolvedPin))
			{
				const FString Before = FormatPinId(ToNodeId, ToPinName);
				const FString After = FormatPinId(ToNodeId, ResolvedName);
				ToPinName = ResolvedName;
				ToPin = ResolvedPin;

				TArray<FString> NodeIds;
				NodeIds.Add(ToNodeId);
				TArray<FString> PinIds;
				PinIds.Add(After);
				RecordAutoFixStep(
					AutoFixState,
					Result,
					TEXT("FIX_PIN_ALIAS"),
					FString::Printf(TEXT("Matched target pin alias on '%s'."), *ToNodeId),
					NodeIds,
					PinIds,
					Before,
					After);

				Result.Warnings.Add(FString::Printf(TEXT("Heuristic pin match applied: %s -> %s"), *Before, *After));
			}
		}

		if (!ToPin && ToNodePtr && *ToNodePtr)
		{
			if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(*ToNodePtr))
			{
				ToPin = FindPinByName(*ToNodePtr, UEdGraphSchema_K2::PN_Execute);
				if (ToPin)
				{
					UE_LOG(LogSOTS_BlueprintGen, Verbose,
						TEXT("ApplyGraphSpec: Link '%s' fell back to Execute pin on result node."),
						*ToNodeId);
				}
			}
		}

		if (!FromPin || !ToPin)
		{
			Result.Warnings.Add(FString::Printf(
				TEXT("Link from '%s.%s' to '%s.%s' could not find pins."),
				*FromNodeId,
				*FromPinName.ToString(),
				*ToNodeId,
				*ToPinName.ToString()));
			AddErrorCode(Result, TEXT("ERR_PIN_NOT_FOUND"));
			continue;
		}

		if (CanAutoFix(AutoFixState) && FromPin->Direction == EGPD_Input && ToPin->Direction == EGPD_Output)
		{
			const FString Before = FString::Printf(TEXT("%s.%s -> %s.%s"), *FromNodeId, *FromPinName.ToString(), *ToNodeId, *ToPinName.ToString());
			Swap(FromPin, ToPin);
			Swap(FromNodeId, ToNodeId);
			Swap(FromPinName, ToPinName);
			const FString After = FString::Printf(TEXT("%s.%s -> %s.%s"), *FromNodeId, *FromPinName.ToString(), *ToNodeId, *ToPinName.ToString());

			TArray<FString> NodeIds;
			NodeIds.Add(FromNodeId);
			NodeIds.Add(ToNodeId);
			TArray<FString> PinIds;
			PinIds.Add(FormatPinId(FromNodeId, FromPinName));
			PinIds.Add(FormatPinId(ToNodeId, ToPinName));
			RecordAutoFixStep(
				AutoFixState,
				Result,
				TEXT("FIX_SWAP_CONNECTION"),
				TEXT("Swapped pin direction to match Output -> Input."),
				NodeIds,
				PinIds,
				Before,
				After);

			Result.Warnings.Add(FString::Printf(TEXT("Heuristic pin match applied: %s -> %s"), *Before, *After));
		}

		FSOTS_BPGenGraphLink EffectiveLink = Link;
		EffectiveLink.FromNodeId = FromNodeId;
		EffectiveLink.ToNodeId = ToNodeId;
		EffectiveLink.FromPinName = FromPinName;
		EffectiveLink.ToPinName = ToPinName;

		if (!ValidateLinkPins(EffectiveLink, FromPin, ToPin, Result))
		{
			AddErrorCode(Result, TEXT("ERR_SCHEMA_REJECTED"));
			continue;
		}

		ConnectPinsWithAutoFix(Blueprint, Graph, EffectiveLink, FromNodeId, ToNodeId, FromPin, ToPin, AutoFixState, Result);
	}
}

static int32 AddPinsFromSpec(UEdGraphNode* Node, EEdGraphPinDirection Direction, const TArray<FSOTS_BPGenPin>& PinDefs, TArray<FString>& OutWarnings)
{
	if (!Node || PinDefs.Num() == 0)
	{
			return 0;
		}

		int32 AddedCount = 0;
		int32 FallbackIndex = 0;
		for (const FSOTS_BPGenPin& PinDef : PinDefs)
		{
			FString PinNameString;
			if (PinDef.Name.IsNone())
			{
				PinNameString = FString::Printf(TEXT("BPGenPin_%d"), ++FallbackIndex);
			}
			else
			{
				PinNameString = PinDef.Name.ToString();
			}

			FEdGraphPinType PinType;
			if (!FillPinTypeFromBPGen(PinDef, PinType))
			{
				OutWarnings.Add(FString::Printf(
					TEXT("ApplyFunctionSkeleton: Pin '%s' has invalid type information."),
					*PinNameString));
				continue;
			}

			UEdGraphPin* NewPin = Node->CreatePin(Direction, PinType, FName(*PinNameString));

			if (!NewPin)
			{
				OutWarnings.Add(FString::Printf(
					TEXT("ApplyFunctionSkeleton: Failed to create pin '%s'."),
					*PinNameString));
				continue;
			}

			if (!PinDef.DefaultValue.IsEmpty())
			{
				NewPin->DefaultValue = PinDef.DefaultValue;
			}

			++AddedCount;
		}

		if (UEdGraph* Graph = Node->GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
		return AddedCount;
	}
}

static FString GetBPGenNodeId(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	return Node->NodeComment;
}

static void SetBPGenNodeId(UEdGraphNode* Node, const FString& NodeId)
{
	if (!Node)
	{
		return;
	}

	Node->NodeComment = NodeId;
}

static FString GetStableNodeId(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	const FString NodeId = GetBPGenNodeId(Node);
	return !NodeId.IsEmpty() ? NodeId : Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
}

static UEdGraphNode* FindNodeByStableId(UEdGraph* Graph, const FString& NodeId)
{
	if (!Graph || NodeId.IsEmpty())
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		if (GetStableNodeId(Node) == NodeId)
		{
			return Node;
		}
	}

	return nullptr;
}

static bool LoadBlueprintAndGraphForEdit(const FString& BlueprintPath, FName FunctionName, UBlueprint*& OutBlueprint, UEdGraph*& OutGraph, TArray<FString>& OutErrors)
{
	OutBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath));
	if (!OutBlueprint)
	{
		OutErrors.Add(FString::Printf(TEXT("Failed to load Blueprint '%s'."), *BlueprintPath));
		return false;
	}

	OutGraph = FindFunctionGraph(OutBlueprint, FunctionName);
	if (!OutGraph)
	{
		OutErrors.Add(FString::Printf(TEXT("Function graph '%s' not found in '%s'."), *FunctionName.ToString(), *BlueprintPath));
		return false;
	}

	return true;
}

static UEdGraphNode* FindNodeByBPGenNodeId(UEdGraph* Graph, const FString& NodeId)
{
	if (!Graph || NodeId.IsEmpty())
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && GetBPGenNodeId(Node) == NodeId)
		{
			return Node;
		}
	}

	return nullptr;
}

template <typename TNode>

static UEdGraphNode* SpawnSelectNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_Select* Node = NewObject<UK2Node_Select>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();

	bool bApplyStructType = false;
	FEdGraphPinType DesiredStructPinType;

	// If a TypePath is provided, try to set the select type before pin allocation so pins get the right schema.
	if (const FString* TypePath = NodeSpec.ExtraData.Find(FName(TEXT("TypePath"))))
	{
		// Prefer structs first (common case), then enums. This avoids noisy log warnings when the path is a struct.
		if (UScriptStruct* StructType = LoadStructFromPath(*TypePath))
		{
			DesiredStructPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			DesiredStructPinType.PinSubCategoryObject = StructType;
			DesiredStructPinType.ContainerType = EPinContainerType::None;
			bApplyStructType = true;
		}
		else if (UEnum* EnumType = LoadObject<UEnum>(nullptr, **TypePath))
		{
			Node->SetEnum(EnumType);
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnSelectNode: Failed to resolve TypePath '%s' for node '%s'."),
				**TypePath, *NodeSpec.Id);
		}
		// If neither enum nor struct, fall back to wildcard and let ExtraData pin overrides shape the type.
	}

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	int32 DesiredOptionPins = 0;
	if (const FString* NumOptionsStr = NodeSpec.ExtraData.Find(FName(TEXT("NumOptionPins"))))
	{
		DesiredOptionPins = FMath::Max(0, FCString::Atoi(**NumOptionsStr));
	}

	if (DesiredOptionPins > 0)
	{
		TArray<UEdGraphPin*> OptionPins;
		Node->GetOptionPins(OptionPins);

		while (OptionPins.Num() < DesiredOptionPins)
		{
			Node->AddInputPin();
			OptionPins.Reset();
			Node->GetOptionPins(OptionPins);
		}
	}

	// Apply index type + default so it does not remain wildcard.
	if (UEdGraphPin* IndexPin = Node->GetIndexPin())
	{
		IndexPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		IndexPin->PinType.PinSubCategory = NAME_None;
		IndexPin->PinType.PinSubCategoryObject = nullptr;
		const FString IndexDefault = NodeSpec.ExtraData.FindRef(FName(TEXT("IndexDefault")));
		IndexPin->DefaultValue = IndexDefault.IsEmpty() ? TEXT("0") : IndexDefault;
		Node->PinTypeChanged(IndexPin);
	}

	if (bApplyStructType)
	{
		TArray<UEdGraphPin*> OptionPins;
		Node->GetOptionPins(OptionPins);
		for (UEdGraphPin* OptionPin : OptionPins)
		{
			if (OptionPin)
			{
				OptionPin->PinType = DesiredStructPinType;
				Node->PinTypeChanged(OptionPin);
			}
		}

		if (UEdGraphPin* ResultPin = Node->GetReturnValuePin())
		{
			ResultPin->PinType = DesiredStructPinType;
			Node->PinTypeChanged(ResultPin);
		}

		if (UEdGraphPin* DefaultPin = Node->FindPin(FName(TEXT("Default"))))
		{
			DefaultPin->PinType = DesiredStructPinType;
			Node->PinTypeChanged(DefaultPin);
		}
	}

	ApplySelectOptionDefaults(Node, NodeSpec);

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

FSOTS_BPGenAssetResult USOTS_BPGenBuilder::CreateStructAssetFromDef(
	const UObject* WorldContextObject,
	const FSOTS_BPGenStructDef& StructDef)
{
	FSOTS_BPGenAssetResult Result;
	Result.AssetPath = StructDef.AssetPath;

	if (StructDef.AssetPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.Message = TEXT("StructDef.AssetPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("CreateStructAssetFromDef failed: AssetPath is empty."));
		return Result;
	}

	const FString PackageName = GetNormalizedPackageName(StructDef.AssetPath);
	const FName StructName = GetSafeObjectName(StructDef.StructName, PackageName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to create or load package '%s'."), *PackageName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	// Ensure the package is fully loaded before we start mutating it so SavePackage won't fail with partial load errors.
	Package->FullyLoad();

	UUserDefinedStruct* TargetStruct = FindObject<UUserDefinedStruct>(Package, *StructName.ToString());
	if (!TargetStruct)
	{
		TargetStruct = FStructureEditorUtils::CreateUserDefinedStruct(
			Package,
			StructName,
			RF_Public | RF_Standalone | RF_Transactional);

		if (!TargetStruct)
		{
			Result.bSuccess = false;
			Result.Message = FString::Printf(TEXT("Failed to create user-defined struct '%s'."), *StructName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
			return Result;
		}
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("CreateStructAssetFromDef: Updating existing struct '%s' in package '%s'."),
			*StructName.ToString(), *PackageName);
	}

	TArray<FStructVariableDescription>& VarDescs = FStructureEditorUtils::GetVarDesc(TargetStruct);
	VarDescs.Reset();

	for (const FSOTS_BPGenPin& MemberPin : StructDef.Members)
	{
		FStructVariableDescription NewVar;
		NewVar.VarGuid = FGuid::NewGuid();
		NewVar.VarName = MemberPin.Name.IsNone() ? FName(TEXT("Member")) : MemberPin.Name;
		NewVar.FriendlyName = NewVar.VarName.ToString();

		FEdGraphPinType PinType;
		if (FillPinTypeFromBPGen(MemberPin, PinType))
		{
			NewVar.SetPinType(PinType);
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("CreateStructAssetFromDef: Invalid pin definition for member '%s'."), *NewVar.VarName.ToString());
		}

		VarDescs.Add(MoveTemp(NewVar));
	}

	FStructureEditorUtils::OnStructureChanged(TargetStruct);

	FAssetRegistryModule::AssetCreated(TargetStruct);
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = false;

	if (!UPackage::SavePackage(Package, TargetStruct, *FileName, SaveArgs))
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to save struct package '%s'."), *FileName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	Result.bSuccess = true;
	Result.Message = FString::Printf(TEXT("Struct '%s' created/updated at '%s'."), *StructName.ToString(), *PackageName);
	return Result;
}

FSOTS_BPGenAssetResult USOTS_BPGenBuilder::CreateEnumAssetFromDef(
	const UObject* WorldContextObject,
	const FSOTS_BPGenEnumDef& EnumDef)
{
	FSOTS_BPGenAssetResult Result;
	Result.AssetPath = EnumDef.AssetPath;

	if (EnumDef.AssetPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.Message = TEXT("EnumDef.AssetPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("CreateEnumAssetFromDef failed: AssetPath is empty."));
		return Result;
	}

	const FString PackageName = GetNormalizedPackageName(EnumDef.AssetPath);
	const FName EnumName = GetSafeObjectName(EnumDef.EnumName, PackageName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to create or load package '%s'."), *PackageName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	// Ensure the package is fully loaded so updates to existing enums operate on a complete asset state.
	Package->FullyLoad();

	UUserDefinedEnum* TargetEnum = FindObject<UUserDefinedEnum>(Package, *EnumName.ToString());
	if (!TargetEnum)
	{
		TargetEnum = NewObject<UUserDefinedEnum>(
			Package,
			EnumName,
			RF_Public | RF_Standalone | RF_Transactional);

		if (!TargetEnum)
		{
			Result.bSuccess = false;
			Result.Message = FString::Printf(TEXT("Failed to create user-defined enum '%s'."), *EnumName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
			return Result;
		}
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("CreateEnumAssetFromDef: Updating existing enum '%s' in package '%s'."),
			*EnumName.ToString(), *PackageName);
	}

	TArray<TPair<FName, int64>> EnumNames;
	for (int32 Index = 0; Index < EnumDef.Values.Num(); ++Index)
	{
		const FString& EntryString = EnumDef.Values[Index];
		const FName EntryName = FName(*EntryString);

		EnumNames.Add(TPair<FName, int64>(EntryName, Index));
	}

	TargetEnum->UEnum::SetEnums(EnumNames, UUserDefinedEnum::ECppForm::Namespaced);
	TargetEnum->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(TargetEnum);
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = false;

	if (!UPackage::SavePackage(Package, TargetEnum, *FileName, SaveArgs))
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to save enum package '%s'."), *FileName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	Result.bSuccess = true;
	Result.Message = FString::Printf(TEXT("Enum '%s' created/updated at '%s'."), *EnumName.ToString(), *PackageName);
	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::ApplyFunctionSkeleton(
	const UObject* WorldContextObject,
	const FSOTS_BPGenFunctionDef& FunctionDef)
{
	FSOTS_BPGenApplyResult Result;
	Result.TargetBlueprintPath = FunctionDef.TargetBlueprintPath;
	Result.FunctionName = FunctionDef.FunctionName;

	if (FunctionDef.TargetBlueprintPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: TargetBlueprintPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	if (FunctionDef.FunctionName.IsNone())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: FunctionName is None.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FunctionDef.TargetBlueprintPath));

	if (!Blueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyFunctionSkeleton: Failed to load Blueprint at '%s'."),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UEdGraph* FunctionGraph = FindFunctionGraph(Blueprint, FunctionDef.FunctionName);

	if (!FunctionGraph)
	{
		// Create a new function graph using the K2 schema.
		FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FunctionDef.FunctionName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		if (!FunctionGraph)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("ApplyFunctionSkeleton: Failed to create function graph '%s'."),
				*FunctionDef.FunctionName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
			return Result;
		}

		FBlueprintEditorUtils::AddFunctionGraph<UFunction>(
			Blueprint,
			FunctionGraph,
			/*bIsUserCreated=*/true,
			nullptr);

		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("ApplyFunctionSkeleton: Created new function graph '%s' in '%s'."),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("ApplyFunctionSkeleton: Reusing existing function graph '%s' in '%s'."),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
	}

	UK2Node_FunctionEntry* EntryNode = nullptr;
	TArray<UK2Node_FunctionResult*> ResultNodes;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
		{
			EntryNode = AsEntry;
		}
		else if (UK2Node_FunctionResult* AsResult = Cast<UK2Node_FunctionResult>(Node))
		{
			ResultNodes.Add(AsResult);
		}
	}

	if (!EntryNode)
	{
		EntryNode = SpawnFunctionEntryNode(FunctionGraph, FVector2D::ZeroVector);
		if (!EntryNode)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: Failed to create function entry node.");
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
			return Result;
		}
	}

	if (ResultNodes.Num() == 0)
	{
		if (UK2Node_FunctionResult* NewResult = SpawnFunctionResultNode(FunctionGraph, FVector2D::ZeroVector))
		{
			ResultNodes.Add(NewResult);
		}
	}

	// Remove any remaining nodes before shaping the entry/result pins so the
	// compiler triggered later doesn't try to recreate stale variable pins.
	{
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			if (Node == EntryNode)
			{
				continue;
			}

			if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				if (ResultNodes.Contains(ResultNode))
				{
					continue;
				}
			}

			NodesToRemove.Add(Node);
		}

		for (UEdGraphNode* NodeToRemove : NodesToRemove)
		{
			FunctionGraph->RemoveNode(NodeToRemove);
		}
	}

	if (ResultNodes.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: Failed to create function result node.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	TArray<FString> PinWarnings;
	ClearNonExecPins(EntryNode);
	const int32 InputPinsAdded = AddPinsFromSpec(EntryNode, EGPD_Input, FunctionDef.Inputs, PinWarnings);
	int32 OutputPinsAdded = 0;
	for (UK2Node_FunctionResult* ResultNode : ResultNodes)
	{
		ClearNonExecPins(ResultNode);
		OutputPinsAdded += AddPinsFromSpec(ResultNode, EGPD_Output, FunctionDef.Outputs, PinWarnings);
	}

	for (const FString& Warning : PinWarnings)
	{
		Result.Warnings.Add(Warning);
	}

	if (FunctionDef.Inputs.Num() > 0 || FunctionDef.Outputs.Num() > 0)
	{
		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("ApplyFunctionSkeleton: Added %d input pin(s) and %d output pin(s) to function '%s'."),
			InputPinsAdded,
			OutputPinsAdded,
			*FunctionDef.FunctionName.ToString());
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ApplyFunctionSkeleton: Failed to save Blueprint after creating function."));
	}

	Result.bSuccess = true;

	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::ApplyGraphSpecToFunction(
	const UObject* WorldContextObject,
	const FSOTS_BPGenFunctionDef& FunctionDef,
	const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult Result;
	Result.TargetBlueprintPath = FunctionDef.TargetBlueprintPath;
	Result.FunctionName = FunctionDef.FunctionName;

	if (FunctionDef.TargetBlueprintPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyGraphSpecToFunction: TargetBlueprintPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	if (FunctionDef.FunctionName.IsNone())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyGraphSpecToFunction: FunctionName is None.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FunctionDef.TargetBlueprintPath));

	if (!Blueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyGraphSpecToFunction: Failed to load Blueprint at '%s'."),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UEdGraph* FunctionGraph = FindFunctionGraph(Blueprint, FunctionDef.FunctionName);
	if (!FunctionGraph)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyGraphSpecToFunction: Function graph '%s' not found in '%s'. Did you call ApplyFunctionSkeleton first?"),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	FSOTS_BPGenGraphSpec CanonicalSpec = CanonicalizeGraphSpecForApply(GraphSpec, Result);
	const FSOTS_BPGenGraphSpec& GraphSpecToUse = CanonicalSpec;
	const EBPGenRepairMode RepairMode = ParseRepairMode(GraphSpecToUse.RepairMode);
	TSet<UEdGraphNode*> ClaimedRepairNodes;

	// Ensure editor-only node modules are loaded so LoadObject lookups succeed when spawning nodes by class path.
	EnsureBlueprintNodeModulesLoaded();

	// Identify existing entry/result nodes before clearing anything.
	UK2Node_FunctionEntry* EntryNode = nullptr;
	TArray<UK2Node_FunctionResult*> ResultNodes;
	TMap<FString, UEdGraphNode*> ExistingNodeIdMap;

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
		{
			if (!EntryNode)
			{
				EntryNode = AsEntry;
			}
		}
		else if (UK2Node_FunctionResult* AsResult = Cast<UK2Node_FunctionResult>(Node))
		{
			ResultNodes.Add(AsResult);
		}

		const FString ExistingId = GetBPGenNodeId(Node);
		if (!ExistingId.IsEmpty())
		{
			ExistingNodeIdMap.Add(ExistingId, Node);
		}
	}

	// Ensure we have entry/result nodes to work with.
	if (!EntryNode)
	{
		EntryNode = SpawnFunctionEntryNode(FunctionGraph, FVector2D::ZeroVector);
	}

	if (ResultNodes.Num() == 0)
	{
		if (UK2Node_FunctionResult* NewResult = SpawnFunctionResultNode(FunctionGraph, FVector2D::ZeroVector))
		{
			ResultNodes.Add(NewResult);
		}
	}

	// Map node ids to spawned/located nodes.
	TMap<FString, UEdGraphNode*> NodeMap;
	TSet<FString> SkippedSpecIds;
	TSet<FString> SkippedNodeIds;

	AddNodeToMap(NodeMap, TEXT("Entry"), EntryNode);
	AddNodeToMap(NodeMap, TEXT("FunctionEntry"), EntryNode);
	if (ResultNodes.Num() > 0)
	{
		AddNodeToMap(NodeMap, TEXT("Result"), ResultNodes[0]);
		AddNodeToMap(NodeMap, TEXT("FunctionResult"), ResultNodes[0]);
	}
	for (int32 Index = 0; Index < ResultNodes.Num(); ++Index)
	{
		AddNodeToMap(NodeMap, FString::Printf(TEXT("Result%d"), Index), ResultNodes[Index]);
	}

	// First, map any spec nodes that reference the existing entry/result nodes.
	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpecToUse.Nodes)
	{
		if (NodeSpec.NodeType == FName(TEXT("K2Node_FunctionEntry")))
		{
			if (EntryNode)
			{
				AddNodeToMap(NodeMap, NodeSpec.Id, EntryNode);
				EntryNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				EntryNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(EntryNode, NodeSpec);

				if (!NodeSpec.NodeId.IsEmpty())
				{
					SetBPGenNodeId(EntryNode, NodeSpec.NodeId);
					Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}
			else
			{
				UE_LOG(LogSOTS_BlueprintGen, Warning,
					TEXT("ApplyGraphSpecToFunction: Node '%s' requests K2Node_FunctionEntry but none found in graph."),
					*NodeSpec.Id);
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_FunctionResult")))
		{
			if (ResultNodes.Num() > 0)
			{
				// For now, bind to the first result node. More complex mappings
				// can be added later if needed.
				AddNodeToMap(NodeMap, NodeSpec.Id, ResultNodes[0]);
				ResultNodes[0]->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				ResultNodes[0]->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(ResultNodes[0], NodeSpec);

				if (!NodeSpec.NodeId.IsEmpty())
				{
					SetBPGenNodeId(ResultNodes[0], NodeSpec.NodeId);
					Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}
			else
			{
				UE_LOG(LogSOTS_BlueprintGen, Warning,
					TEXT("ApplyGraphSpecToFunction: Node '%s' requests K2Node_FunctionResult but none found in graph."),
					*NodeSpec.Id);
			}
		}
	}

	// Spawn new nodes for all remaining specs.
	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpecToUse.Nodes)
	{
		if (NodeMap.Contains(NodeSpec.Id))
		{
			continue; // Already mapped (entry/result).
		}

		UEdGraphNode* NewNode = nullptr;
		const bool bHasNodeId = !NodeSpec.NodeId.IsEmpty();
		UEdGraphNode* ExistingNode = bHasNodeId ? ExistingNodeIdMap.FindRef(NodeSpec.NodeId) : nullptr;
		if (!ExistingNode && bHasNodeId && RepairMode != EBPGenRepairMode::None && NodeSpec.bCreateOrUpdate && NodeSpec.bAllowUpdate)
		{
			FString RepairDescription;
			ExistingNode = FindRepairCandidate(NodeSpec, FunctionGraph->Nodes, ClaimedRepairNodes, RepairMode, RepairDescription);
			if (ExistingNode)
			{
				SetBPGenNodeId(ExistingNode, NodeSpec.NodeId);
				ExistingNodeIdMap.Add(NodeSpec.NodeId, ExistingNode);
				ClaimedRepairNodes.Add(ExistingNode);

				FSOTS_BPGenRepairStep Step;
				Step.StepIndex = Result.RepairSteps.Num();
				Step.Code = TEXT("REPAIR_NODE_ID");
				Step.Description = RepairDescription;
				Step.AffectedNodeIds.Add(NodeSpec.NodeId);
				Step.Before = TEXT("node_id missing");
				Step.After = NodeSpec.NodeId;
				Result.RepairSteps.Add(Step);
			}
		}
		const bool bAllowCreate = NodeSpec.bAllowCreate;
		const bool bAllowUpdate = NodeSpec.bAllowUpdate;
		const bool bCreateOrUpdate = NodeSpec.bCreateOrUpdate;
		const bool bReuseExistingNode = ExistingNode && bCreateOrUpdate && bAllowUpdate;

		if (bReuseExistingNode)
		{
			NewNode = ExistingNode;
			NewNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
			NewNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
			ApplyExtraPinDefaults(NewNode, NodeSpec);

			if (bHasNodeId)
			{
				SetBPGenNodeId(NewNode, NodeSpec.NodeId);
				Result.UpdatedNodeIds.AddUnique(NodeSpec.NodeId);
			}
		}
		else if (ExistingNode && bCreateOrUpdate && !bAllowUpdate)
		{
			if (bHasNodeId)
			{
				Result.SkippedNodeIds.AddUnique(NodeSpec.NodeId);
				SkippedNodeIds.Add(NodeSpec.NodeId);
			}

			SkippedSpecIds.Add(NodeSpec.Id);
			continue;
		}

		if (!NewNode && (!bAllowCreate && !bHasNodeId))
		{
			if (bHasNodeId)
			{
				Result.SkippedNodeIds.AddUnique(NodeSpec.NodeId);
				SkippedNodeIds.Add(NodeSpec.NodeId);
			}

			SkippedSpecIds.Add(NodeSpec.Id);
			continue;
		}

		if (!NewNode && ExistingNode && !bCreateOrUpdate)
		{
			Result.Warnings.Add(FString::Printf(
				TEXT("NodeId '%s' already exists but create_or_update=false; spawning a new node and leaving the existing one untouched."),
				*NodeSpec.NodeId));
		}

		if (!NewNode)
		{
			const bool bHasSpawnerKey = !NodeSpec.SpawnerKey.IsEmpty();
			if (bHasSpawnerKey && NodeSpec.bPreferSpawnerKey)
			{
				NewNode = SpawnNodeFromSpawnerKey(Blueprint, FunctionGraph, NodeSpec, Result);
			}

			if (!NewNode)
			{
				if (NodeSpec.NodeType == FName(TEXT("K2Node_Knot")))
				{
					NewNode = SpawnKnotNode(FunctionGraph, NodeSpec);
				}
				else if (NodeSpec.NodeType == FName(TEXT("K2Node_Select")))
				{
					NewNode = SpawnSelectNode(FunctionGraph, NodeSpec);
					if (!NewNode)
					{
						Result.Warnings.Add(FString::Printf(
							TEXT("Failed to spawn K2Node_Select for node '%s'."),
							*NodeSpec.Id));
					}
				}
				else if (NodeSpec.NodeType == FName(TEXT("K2Node_DynamicCast")))
				{
					NewNode = SpawnDynamicCastNode(FunctionGraph, NodeSpec);
					if (!NewNode)
					{
						Result.Warnings.Add(FString::Printf(
							TEXT("Failed to spawn K2Node_DynamicCast for node '%s'."),
							*NodeSpec.Id));
					}
				}
				else
				{
					UE_LOG(LogSOTS_BlueprintGen, Warning,
						TEXT("ApplyGraphSpecToFunction: NodeType '%s' requires a spawner_key; skipping node '%s'."),
						*NodeSpec.NodeType.ToString(), *NodeSpec.Id);
					Result.Warnings.Add(FString::Printf(
						TEXT("NodeType '%s' not supported; provide a spawner_key for node '%s'."),
						*NodeSpec.NodeType.ToString(), *NodeSpec.Id));
				}
			}
		}

		if (NewNode)
		{
			if (bHasNodeId)
			{
				SetBPGenNodeId(NewNode, NodeSpec.NodeId);
				if (!bReuseExistingNode)
				{
					Result.CreatedNodeIds.AddUnique(NodeSpec.NodeId);
				}
			}

			AddNodeToMap(NodeMap, NodeSpec.Id, NewNode);
		}
	}

	// Connect pins according to Links.
	ApplyGraphLinks(Blueprint, FunctionGraph, GraphSpecToUse, NodeMap, SkippedSpecIds, Result);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ApplyGraphSpecToFunction: Failed to save Blueprint after applying graph."));
	}

	Result.ChangeSummary.BlueprintAssetPath = FunctionDef.TargetBlueprintPath;
	Result.ChangeSummary.TargetType = TEXT("Function");
	Result.ChangeSummary.TargetName = FunctionDef.FunctionName.ToString();
	Result.ChangeSummary.CreatedNodeIds = Result.CreatedNodeIds;
	Result.ChangeSummary.UpdatedNodeIds = Result.UpdatedNodeIds;

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenGraphEditResult USOTS_BPGenBuilder::DeleteNodeById(const UObject* WorldContextObject, const FSOTS_BPGenDeleteNodeRequest& Request)
{
	FSOTS_BPGenGraphEditResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.FunctionName = Request.FunctionName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.NodeId.IsEmpty())
	{
		Result.Errors.Add(TEXT("DeleteNodeById: blueprint_asset_path, function_name, and node_id are required."));
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "DeleteNode", "SOTS BPGen: Delete Node"));

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!LoadBlueprintAndGraphForEdit(Request.BlueprintAssetPath, Request.FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	UEdGraphNode* TargetNode = FindNodeByStableId(FunctionGraph, Request.NodeId);
	if (!TargetNode)
	{
		Result.Errors.Add(FString::Printf(TEXT("DeleteNodeById: NodeId '%s' not found."), *Request.NodeId));
		return Result;
	}

	FunctionGraph->Modify();
	TargetNode->Modify();
	TargetNode->BreakAllNodeLinks();
	FunctionGraph->RemoveNode(TargetNode);
	FunctionGraph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	if (Request.bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("DeleteNodeById: Failed to save Blueprint after deletion."));
	}

	Result.AffectedNodeIds.Add(Request.NodeId);
	Result.bSuccess = Result.Errors.Num() == 0;
	Result.Message = FString::Printf(TEXT("Deleted node '%s'."), *Request.NodeId);
	return Result;
}

FSOTS_BPGenGraphEditResult USOTS_BPGenBuilder::DeleteLink(const UObject* WorldContextObject, const FSOTS_BPGenDeleteLinkRequest& Request)
{
	FSOTS_BPGenGraphEditResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.FunctionName = Request.FunctionName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.FromNodeId.IsEmpty() || Request.ToNodeId.IsEmpty())
	{
		Result.Errors.Add(TEXT("DeleteLink: blueprint_asset_path, function_name, from_node_id, and to_node_id are required."));
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "DeleteLink", "SOTS BPGen: Delete Link"));

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!LoadBlueprintAndGraphForEdit(Request.BlueprintAssetPath, Request.FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	UEdGraphNode* FromNode = FindNodeByStableId(FunctionGraph, Request.FromNodeId);
	UEdGraphNode* ToNode = FindNodeByStableId(FunctionGraph, Request.ToNodeId);
	if (!FromNode || !ToNode)
	{
		if (!FromNode)
		{
			Result.Errors.Add(FString::Printf(TEXT("DeleteLink: FromNodeId '%s' not found."), *Request.FromNodeId));
		}
		if (!ToNode)
		{
			Result.Errors.Add(FString::Printf(TEXT("DeleteLink: ToNodeId '%s' not found."), *Request.ToNodeId));
		}
		return Result;
	}

	UEdGraphPin* FromPin = FindPinByName(FromNode, Request.FromPinName);
	UEdGraphPin* ToPin = FindPinByName(ToNode, Request.ToPinName);

	if (!FromPin || !ToPin)
	{
		if (!FromPin)
		{
			Result.Errors.Add(FString::Printf(TEXT("DeleteLink: Pin '%s' not found on node '%s'."), *Request.FromPinName.ToString(), *Request.FromNodeId));
		}
		if (!ToPin)
		{
			Result.Errors.Add(FString::Printf(TEXT("DeleteLink: Pin '%s' not found on node '%s'."), *Request.ToPinName.ToString(), *Request.ToNodeId));
		}
		return Result;
	}

	const bool bHadLink = FromPin->LinkedTo.Contains(ToPin);
	FromPin->BreakLinkTo(ToPin);

	if (!bHadLink)
	{
		Result.Warnings.Add(TEXT("DeleteLink: Link did not exist; nothing to remove."));
	}

	FunctionGraph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	if (Request.bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("DeleteLink: Failed to save Blueprint after removing link."));
	}

	Result.AffectedNodeIds.Add(Request.FromNodeId);
	Result.AffectedNodeIds.Add(Request.ToNodeId);
	Result.bSuccess = Result.Errors.Num() == 0;
	Result.Message = FString::Printf(TEXT("Removed link %s.%s -> %s.%s."), *Request.FromNodeId, *Request.FromPinName.ToString(), *Request.ToNodeId, *Request.ToPinName.ToString());
	return Result;
}

FSOTS_BPGenGraphEditResult USOTS_BPGenBuilder::ReplaceNodePreserveId(const UObject* WorldContextObject, const FSOTS_BPGenReplaceNodeRequest& Request)
{
	FSOTS_BPGenGraphEditResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.FunctionName = Request.FunctionName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.ExistingNodeId.IsEmpty())
	{
		Result.Errors.Add(TEXT("ReplaceNodePreserveId: blueprint_asset_path, function_name, and existing_node_id are required."));
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "ReplaceNode", "SOTS BPGen: Replace Node"));

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!LoadBlueprintAndGraphForEdit(Request.BlueprintAssetPath, Request.FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	UEdGraphNode* ExistingNode = FindNodeByStableId(FunctionGraph, Request.ExistingNodeId);
	if (!ExistingNode)
	{
		Result.Errors.Add(FString::Printf(TEXT("ReplaceNodePreserveId: NodeId '%s' not found."), *Request.ExistingNodeId));
		return Result;
	}

	struct FPendingReconnect
	{
		bool bOldWasSource = false;
		FString OtherNodeId;
		FName OtherPinName;
		FName OldPinName;
	};

	TArray<FPendingReconnect> PendingLinks;
	for (UEdGraphPin* Pin : ExistingNode->Pins)
	{
		if (!Pin)
		{
			continue;
		}

		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (!LinkedPin)
			{
				continue;
			}

			FPendingReconnect& Pending = PendingLinks.AddDefaulted_GetRef();
			Pending.bOldWasSource = (Pin->Direction == EGPD_Output);
			Pending.OtherNodeId = GetStableNodeId(LinkedPin->GetOwningNode());
			Pending.OtherPinName = LinkedPin->PinName;
			Pending.OldPinName = Pin->PinName;
		}
	}

	const FVector2D OldPosition(ExistingNode->NodePosX, ExistingNode->NodePosY);
	FunctionGraph->Modify();
	ExistingNode->Modify();
	ExistingNode->BreakAllNodeLinks();
	FunctionGraph->RemoveNode(ExistingNode);

	FSOTS_BPGenGraphNode NewNodeSpec = Request.NewNode;
	if (NewNodeSpec.NodeId.IsEmpty())
	{
		NewNodeSpec.NodeId = Request.ExistingNodeId;
	}
	if (NewNodeSpec.NodePosition.IsNearlyZero())
	{
		NewNodeSpec.NodePosition = OldPosition;
	}

	FSOTS_BPGenApplyResult TempApplyResult;
	UEdGraphNode* NewNode = nullptr;
	if (!NewNodeSpec.SpawnerKey.IsEmpty() && NewNodeSpec.bPreferSpawnerKey)
	{
		NewNode = SpawnNodeFromSpawnerKey(Blueprint, FunctionGraph, NewNodeSpec, TempApplyResult);
	}

	if (!NewNode)
	{
		if (NewNodeSpec.NodeType == FName(TEXT("K2Node_Knot")))
		{
			NewNode = SpawnKnotNode(FunctionGraph, NewNodeSpec);
		}
		else if (NewNodeSpec.NodeType == FName(TEXT("K2Node_Select")))
		{
			NewNode = SpawnSelectNode(FunctionGraph, NewNodeSpec);
		}
		else if (NewNodeSpec.NodeType == FName(TEXT("K2Node_DynamicCast")))
		{
			NewNode = SpawnDynamicCastNode(FunctionGraph, NewNodeSpec);
		}
		else
		{
			TempApplyResult.Errors.Add(FString::Printf(TEXT("ReplaceNodePreserveId: Unsupported node_type '%s'. Provide a spawner_key."), *NewNodeSpec.NodeType.ToString()));
		}
	}

	if (!NewNode)
	{
		Result.Errors.Append(TempApplyResult.Errors);
		Result.Warnings.Append(TempApplyResult.Warnings);
		return Result;
	}

	SetBPGenNodeId(NewNode, NewNodeSpec.NodeId);
	Result.AffectedNodeIds.Add(NewNodeSpec.NodeId);

	for (const FPendingReconnect& Link : PendingLinks)
	{
		const FName* RemappedPin = Request.PinRemap.Find(Link.OldPinName);
		const FName TargetPinName = RemappedPin ? *RemappedPin : Link.OldPinName;

		UEdGraphPin* NewPin = FindPinByName(NewNode, TargetPinName);
		if (!NewPin)
		{
			Result.Warnings.Add(FString::Printf(TEXT("ReplaceNodePreserveId: Pin '%s' not found on replacement node."), *TargetPinName.ToString()));
			continue;
		}

		UEdGraphNode* OtherNode = FindNodeByStableId(FunctionGraph, Link.OtherNodeId);
		if (!OtherNode)
		{
			Result.Warnings.Add(FString::Printf(TEXT("ReplaceNodePreserveId: Linked node '%s' not found; skipping reconnect."), *Link.OtherNodeId));
			continue;
		}

		UEdGraphPin* OtherPin = FindPinByName(OtherNode, Link.OtherPinName);
		if (!OtherPin)
		{
			Result.Warnings.Add(FString::Printf(TEXT("ReplaceNodePreserveId: Pin '%s' not found on linked node '%s'."), *Link.OtherPinName.ToString(), *Link.OtherNodeId));
			continue;
		}

		FSOTS_BPGenGraphLink LinkSpec;
		if (Link.bOldWasSource)
		{
			LinkSpec.FromNodeId = NewNodeSpec.NodeId;
			LinkSpec.FromPinName = TargetPinName;
			LinkSpec.ToNodeId = Link.OtherNodeId;
			LinkSpec.ToPinName = Link.OtherPinName;
		}
		else
		{
			LinkSpec.FromNodeId = Link.OtherNodeId;
			LinkSpec.FromPinName = Link.OtherPinName;
			LinkSpec.ToNodeId = NewNodeSpec.NodeId;
			LinkSpec.ToPinName = TargetPinName;
		}

		if (!ValidateLinkPins(LinkSpec, Link.bOldWasSource ? NewPin : OtherPin, Link.bOldWasSource ? OtherPin : NewPin, TempApplyResult))
		{
			continue;
		}

		ConnectPinsSchemaFirst(LinkSpec, Link.bOldWasSource ? NewPin : OtherPin, Link.bOldWasSource ? OtherPin : NewPin, TempApplyResult);
	}

	FunctionGraph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	if (Request.bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ReplaceNodePreserveId: Failed to save Blueprint after replacement."));
	}

	Result.Errors.Append(TempApplyResult.Errors);
	Result.Warnings.Append(TempApplyResult.Warnings);
	Result.bSuccess = Result.Errors.Num() == 0;
	Result.Message = FString::Printf(TEXT("Replaced node '%s' and preserved NodeId."), *Request.ExistingNodeId);
	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::BuildTestAllNodesGraphForBPPrintHello()
{
	// Discovery → spawner_key → schema-connect harness. The legacy “all nodes” fixture is deprecated.
	static const TCHAR* TestBlueprintPath = TEXT("/Game/DevTools/BP_PrintHello.BP_PrintHello");
	static const FName TestFunctionName(TEXT("Test_AllNodesGraph"));

	FSOTS_BPGenFunctionDef FunctionDef;
	FunctionDef.TargetBlueprintPath = TestBlueprintPath;
	FunctionDef.FunctionName = TestFunctionName;

	FSOTS_BPGenApplyResult SkeletonResult = ApplyFunctionSkeleton(nullptr, FunctionDef);
	if (!SkeletonResult.bSuccess)
	{
		return SkeletonResult;
	}

	// 1) “Discovery”: assemble a descriptor as if returned by a discovery pass.
	FSOTS_BPGenNodeSpawnerDescriptor PrintStringDescriptor;
	PrintStringDescriptor.SpawnerKey = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
	PrintStringDescriptor.DisplayName = TEXT("Print String");
	PrintStringDescriptor.NodeClassName = TEXT("K2Node_CallFunction");
	PrintStringDescriptor.FunctionPath = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
	PrintStringDescriptor.NodeType = TEXT("function_call");

	// 2) Translate descriptor → graph node spec with explicit NodeId and position.
	FSOTS_BPGenGraphNode PrintStringNode = USOTS_BPGenDescriptorTranslator::MakeGraphNodeFromDescriptor(
		PrintStringDescriptor,
		TEXT("PrintString"));
	PrintStringNode.bPreferSpawnerKey = true;
	PrintStringNode.NodePosition = FVector2D(300.f, 0.f);
	PrintStringNode.ExtraData.Add(TEXT("InString.DefaultValue"), TEXT("BPGen Spawner Harness"));

	// 3) Build schema-first links using the helper for clarity.
	FSOTS_BPGenGraphSpec GraphSpec;
	GraphSpec.Nodes.Add(PrintStringNode);
	GraphSpec.Links.Add(USOTS_BPGenDescriptorTranslator::MakeLinkSpec(
		TEXT("Entry"),
		UEdGraphSchema_K2::PN_Then,
		PrintStringNode.Id,
		UEdGraphSchema_K2::PN_Execute,
		/*bUseSchema=*/true));
	GraphSpec.Links.Add(USOTS_BPGenDescriptorTranslator::MakeLinkSpec(
		PrintStringNode.Id,
		UEdGraphSchema_K2::PN_Then,
		TEXT("Result"),
		UEdGraphSchema_K2::PN_Execute,
		/*bUseSchema=*/true));

	FSOTS_BPGenApplyResult GraphResult = ApplyGraphSpecToFunction(nullptr, FunctionDef, GraphSpec);
	GraphResult.Warnings.Append(SkeletonResult.Warnings);
	return GraphResult;
}
