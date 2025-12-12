#include "BlueprintFlowExporter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"

#include "BEP.h"

	namespace
	{
	IAssetRegistry& GetAssetRegistry()
	{
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		return AssetRegistryModule.Get();
	}

	void DumpGraphText(UBlueprint* Blueprint, UEdGraph* Graph, FString& OutText)
	{
		if (!Graph)
		{
			return;
		}

		OutText += FString::Printf(TEXT("  Graph: %s\n"), *Graph->GetName());

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			const FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
			OutText += FString::Printf(TEXT("    Node: %s (%s)\n"),
				*NodeTitle,
				*Node->GetClass()->GetName());

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin)
				{
					continue;
				}

				const TCHAR* DirectionText = Pin->Direction == EGPD_Input ? TEXT("In") : TEXT("Out");
				OutText += FString::Printf(TEXT("      Pin: %s (%s) Dir=%s\n"),
					*Pin->PinName.ToString(),
					*Pin->PinType.PinCategory.ToString(),
					DirectionText);

				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					if (!LinkedPin || !LinkedPin->GetOwningNode())
					{
						continue;
					}

					const FString LinkedNodeTitle =
						LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

					OutText += FString::Printf(TEXT("        -> %s.%s\n"),
						*LinkedNodeTitle,
						*LinkedPin->PinName.ToString());
				}
			}
		}
	}

	void LogFileWriteFailure(const FString& FilePath, const TCHAR* Context)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
		const FString Directory = FPaths::GetPath(FullPath);
		const bool bDirExists = IFileManager::Get().DirectoryExists(*Directory);
		const bool bIsReadOnly = IFileManager::Get().IsReadOnly(*FullPath);

		UE_LOG(
			LogBEP,
			Warning,
			TEXT("[BEP] Failed to write file (%s)\n  FilePath: %s\n  FullPath: %s\n  Directory: %s (Exists=%s)\n  ReadOnly=%s"),
			Context ? Context : TEXT("NoContext"),
			*FilePath,
			*FullPath,
			*Directory,
			bDirExists ? TEXT("true") : TEXT("false"),
			bIsReadOnly ? TEXT("true") : TEXT("false"));
	}

	bool PropertyContainsUnsupportedLocator(FProperty* Prop);

	bool StructContainsUnsupportedLocator(UStruct* InStruct)
	{
		if (!InStruct)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(InStruct, EFieldIterationFlags::IncludeSuper); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			if (PropertyContainsUnsupportedLocator(Prop))
			{
				return true;
			}
		}

		return false;
	}

	bool PropertyContainsUnsupportedLocator(FProperty* Prop)
	{
		if (!Prop)
		{
			return false;
		}

		// Direct struct property.
		if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			if (UScriptStruct* InnerStruct = StructProp->Struct)
			{
				const FString StructName = InnerStruct->GetName();

				// Universal Object Locator / Actor locator fragments currently assert
				// in ToString when they contain certain characters. Rather than
				// crashing the editor during export, we conservatively skip any
				// asset whose class has such a property and log it.
				if (StructName.Contains(TEXT("UniversalObjectLocator")) ||
					StructName.Contains(TEXT("ActorLocatorFragment")))
				{
					return true;
				}

				if (StructContainsUnsupportedLocator(InnerStruct))
				{
					return true;
				}
			}
		}

		// Arrays of structs / locators.
		if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			return PropertyContainsUnsupportedLocator(ArrayProp->Inner);
		}

		// Maps whose key or value is or contains a locator struct.
		if (FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		{
			return PropertyContainsUnsupportedLocator(MapProp->KeyProp) ||
			       PropertyContainsUnsupportedLocator(MapProp->ValueProp);
		}

		return false;
	}

	void EnsureDirectory(const FString& Dir)
	{
		if (!Dir.IsEmpty())
		{
			const FString FullDir = FPaths::ConvertRelativePathToFull(Dir);

			const bool bOk = IFileManager::Get().MakeDirectory(*FullDir, true);
			if (!bOk)
			{
				const bool bDirExists = IFileManager::Get().DirectoryExists(*FullDir);

				UE_LOG(
					LogBEP,
					Warning,
					TEXT("[BEP] Failed to create directory\n  DirArg: %s\n  FullDir: %s\n  AlreadyExists=%s"),
					*Dir,
					*FullDir,
					bDirExists ? TEXT("true") : TEXT("false"));
			}
			else
			{
				UE_LOG(
					LogBEP,
					Verbose,
					TEXT("[BEP] Ensured directory exists\n  DirArg: %s\n  FullDir: %s"),
					*Dir,
					*FullDir);
			}
		}
	}
}

static FString ToCsvSafe(const FString& In)
{
	FString Result = In;
	Result.ReplaceInline(TEXT("\""), TEXT("\"\""));
	return FString::Printf(TEXT("\"%s\""), *Result);
}

void FBlueprintFlowExporter::ExportGraphSnippet(UBlueprint* Blueprint, UEdGraph* Graph, const FString& SnippetOutputDirectory)
{
	if (!Blueprint || !Graph)
	{
		return;
	}

	FGraphPanelSelectionSet Selection;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			Selection.Add(Node);
		}
	}

	if (Selection.Num() == 0)
	{
		return;
	}

	FString SnippetText;
	FEdGraphUtilities::ExportNodesToText(Selection, SnippetText);
	if (SnippetText.IsEmpty())
	{
		return;
	}

	FString FileName = FString::Printf(TEXT("%s_%s_Snippet.txt"), *Blueprint->GetName(), *Graph->GetName());
	FPaths::MakeValidFileName(FileName);

	const FString OutputPath = FPaths::Combine(SnippetOutputDirectory, FileName);
	IFileManager::Get().MakeDirectory(*SnippetOutputDirectory, true);

	FFileHelper::SaveStringToFile(
		SnippetText,
		*OutputPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString FBlueprintFlowExporter::GetPinDefaultValueString(UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return FString();
	}

	if (Pin->DefaultObject)
	{
		return Pin->DefaultObject->GetName();
	}

	if (!Pin->DefaultTextValue.IsEmpty())
	{
		return Pin->DefaultTextValue.ToString();
	}

	if (!Pin->DefaultValue.IsEmpty())
	{
		return Pin->DefaultValue;
	}

	return FString();
}

FString FBlueprintFlowExporter::DescribePinResolvedValue(UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return FString();
	}

	if (Pin->LinkedTo.Num() == 0)
	{
		return GetPinDefaultValueString(Pin);
	}

	UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
	if (!LinkedPin)
	{
		return FString();
	}

	UEdGraphNode* SourceNode = LinkedPin->GetOwningNode();
	if (!SourceNode)
	{
		return FString();
	}

	const FString SourceTitle = SourceNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

	const FString SourcePinName =
		LinkedPin->PinFriendlyName.IsEmpty()
			? LinkedPin->PinName.ToString()
			: LinkedPin->PinFriendlyName.ToString();

	if (SourcePinName.IsEmpty() || SourcePinName.Equals(TEXT("ReturnValue"), ESearchCase::IgnoreCase))
	{
		return SourceTitle;
	}

	return FString::Printf(TEXT("%s.%s"), *SourceTitle, *SourcePinName);
}

void FBlueprintFlowExporter::GetAllGraphsForBlueprint(UBlueprint* Blueprint, TArray<UEdGraph*>& OutGraphs)
{
	OutGraphs.Empty();

	if (!Blueprint)
	{
		return;
	}

	auto AddGraphIfUnique = [&OutGraphs](UEdGraph* Graph)
	{
		if (Graph && !OutGraphs.Contains(Graph))
		{
			OutGraphs.Add(Graph);
		}
	};

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		AddGraphIfUnique(Graph);
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		AddGraphIfUnique(Graph);
	}

	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		AddGraphIfUnique(Graph);
	}

	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
	{
		AddGraphIfUnique(Graph);
	}

	// Optional: add IntermediateGeneratedGraphs if desired (can be noisy).
	// for (UEdGraph* Graph : Blueprint->IntermediateGeneratedGraphs)
	// {
	// 	if (Graph)
	// 	{
	// 		OutGraphs.Add(Graph);
	// 	}
	// }
}

void FBlueprintFlowExporter::ParseExcludedPatterns(const FString& PatternsString, TArray<FString>& OutPatterns)
{
	OutPatterns.Empty();

	TArray<FString> Tokens;
	PatternsString.ParseIntoArray(Tokens, TEXT(","), true);

	if (Tokens.Num() <= 1 && PatternsString.Contains(TEXT("\n")))
	{
		Tokens.Reset();
		PatternsString.ParseIntoArrayLines(Tokens);
	}

	for (FString& Token : Tokens)
	{
		Token.TrimStartAndEndInline();
		if (!Token.IsEmpty())
		{
			OutPatterns.Add(Token);
		}
	}
}

const TSet<FName>& FBlueprintFlowExporter::GetDefaultExcludedClassNames()
{
	static const TSet<FName> DefaultExcludedClasses = {
		// Textures
		FName(TEXT("Texture")),
		FName(TEXT("Texture2D")),
		FName(TEXT("TextureCube")),
		FName(TEXT("TextureRenderTarget2D")),
		FName(TEXT("TextureRenderTargetCube")),
		FName(TEXT("VirtualTexture2D")),

		// Materials
		FName(TEXT("Material")),
		FName(TEXT("MaterialInstanceConstant")),
		FName(TEXT("MaterialInstanceDynamic")),
		FName(TEXT("MaterialFunction")),
		FName(TEXT("MaterialFunctionInstance")),
		FName(TEXT("MaterialParameterCollection")),
		FName(TEXT("PhysicalMaterial")),

		// Meshes
		FName(TEXT("StaticMesh")),
		FName(TEXT("SkeletalMesh")),
		FName(TEXT("SkeletalMeshLODSettings")),
		FName(TEXT("Skeleton")),
		FName(TEXT("GeometryCache")),
		FName(TEXT("GeometryCollection")),

		// Raw animation assets (keep AnimBlueprints elsewhere)
		FName(TEXT("AnimSequence")),
		FName(TEXT("AnimComposite")),
		FName(TEXT("AnimMontage")),
		FName(TEXT("BlendSpace")),
		FName(TEXT("BlendSpace1D")),
		FName(TEXT("AimOffsetBlendSpace")),
		FName(TEXT("AimOffsetBlendSpace1D")),
		FName(TEXT("AnimCurve")),
		FName(TEXT("AnimData")),
		FName(TEXT("AnimStreamable")),

		// FX / particles
		FName(TEXT("NiagaraSystem")),
		FName(TEXT("NiagaraEmitter")),
		FName(TEXT("ParticleSystem")),

		// Audio
		FName(TEXT("SoundWave")),
		FName(TEXT("SoundCue")),
		FName(TEXT("MetaSoundSource")),
		FName(TEXT("MetaSoundPatch")),

		// Misc non-graph assets
		FName(TEXT("LandscapeLayerInfoObject")),
		FName(TEXT("PhysicsAsset")),
		FName(TEXT("DestructibleMesh")),
		FName(TEXT("DataLayerAsset"))
	};

	return DefaultExcludedClasses;
}

bool FBlueprintFlowExporter::ShouldSkipAsset(const FAssetData& AssetData, const TArray<FString>& UserExcludedClassPatterns)
{
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	const FName ClassName = AssetData.AssetClassPath.GetAssetName();
#else
	const FName ClassName = AssetData.AssetClass;
#endif

	const FString ClassNameStr = ClassName.ToString();

	if (GetDefaultExcludedClassNames().Contains(ClassName))
	{
		return true;
	}

	for (const FString& Pattern : UserExcludedClassPatterns)
	{
		if (Pattern.IsEmpty())
		{
			continue;
		}

		if (ClassNameStr.Contains(Pattern, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void FBlueprintFlowExporter::ExportAllForFormat(const FString& OutputRoot, const FString& RootPath, EBEPExportOutputFormat OutputFormat, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun)
{
	// ExcludedClassPatterns combine defaults and user-provided strings; used by ShouldSkipAsset before loading assets.
	if (ExcludedClassPatterns.Num() > 0)
	{
		UE_LOG(LogBEP, Log, TEXT("[BEP] Excluding class patterns: %s"), *FString::Join(ExcludedClassPatterns, TEXT(", ")));
	}

	auto RunExport = [&](EBEPExportFormat Format)
	{
		ExportAll(OutputRoot, RootPath, Format, ExcludedClassPatterns, MaxAssetsPerRun);
	};

	switch (OutputFormat)
	{
	case EBEPExportOutputFormat::Text:
		RunExport(EBEPExportFormat::Text);
		break;
	case EBEPExportOutputFormat::Json:
		RunExport(EBEPExportFormat::Json);
		break;
	case EBEPExportOutputFormat::Csv:
		RunExport(EBEPExportFormat::Csv);
		break;
	case EBEPExportOutputFormat::All:
		RunExport(EBEPExportFormat::Text);
		RunExport(EBEPExportFormat::Json);
		RunExport(EBEPExportFormat::Csv);
		break;
	default:
		break;
	}
}

void FBlueprintFlowExporter::ExportWithSettings(const FBEPExportSettings& Settings)
{
	if (Settings.RootPath.IsEmpty())
	{
		UE_LOG(LogBEP, Warning, TEXT("[BEP] ExportWithSettings: RootPath is empty."));
		return;
	}

	if (Settings.OutputRootPath.IsEmpty())
	{
		UE_LOG(LogBEP, Warning, TEXT("[BEP] ExportWithSettings: OutputRootPath is empty."));
		return;
	}

	TArray<FString> ExcludedPatterns;
	ParseExcludedPatterns(Settings.ExcludedClassPatterns, ExcludedPatterns);

	IFileManager::Get().MakeDirectory(*Settings.OutputRootPath, true);

	const int32 MaxAssets = Settings.MaxAssetsPerRun > 0 ? Settings.MaxAssetsPerRun : 500;
	ExportAllForFormat(Settings.OutputRootPath, Settings.RootPath, Settings.OutputFormat, ExcludedPatterns, MaxAssets);
}

void FBlueprintFlowExporter::ExportAllBlueprintFlows(const FString& OutputDirectory, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();

	FARFilter Filter;
	// Include UBlueprint and all derived types (e.g. UWidgetBlueprint) so that
	// regular Blueprints **and** UMG widget Blueprints are exported.
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(*RootPath);
	Filter.bRecursivePaths = true;

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	const int32 TotalAssets = BlueprintAssets.Num();
	const int32 Limit = (MaxAssetsPerRun > 0) ? FMath::Min(MaxAssetsPerRun, TotalAssets) : TotalAssets;
	const int32 GCInterval = 50;

	const FString OutDir = OutputDirectory.IsEmpty()
		                        ? (FPaths::ProjectSavedDir() / TEXT("BEPExport/BlueprintFlows"))
		                        : OutputDirectory;
	EnsureDirectory(OutDir);
	const FString SnippetDir = OutDir / TEXT("Snippets");

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllBlueprintFlows: RootPath=%s AssetsFound=%d Limit=%d OutputDir=%s Format=%d"),
		*RootPath, BlueprintAssets.Num(), Limit, *OutDir, static_cast<int32>(Format));

	int32 ExportedCount = 0;

	for (int32 Index = 0; Index < Limit; ++Index)
	{
		const FAssetData& Asset = BlueprintAssets[Index];
		if (ShouldSkipAsset(Asset, ExcludedClassPatterns))
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		const FString SafeName = Asset.AssetName.ToString().Replace(TEXT("/"), TEXT("_"));

		TArray<UEdGraph*> Graphs;
		GetAllGraphsForBlueprint(Blueprint, Graphs);

		auto GetGraphTypeString = [Blueprint](UEdGraph* Graph)
		{
			if (Blueprint->UbergraphPages.Contains(Graph))
			{
				return FString(TEXT("Ubergraph"));
			}
			if (Blueprint->FunctionGraphs.Contains(Graph))
			{
				return FString(TEXT("Function"));
			}
			if (Blueprint->MacroGraphs.Contains(Graph))
			{
				return FString(TEXT("Macro"));
			}
			if (Blueprint->DelegateSignatureGraphs.Contains(Graph))
			{
				return FString(TEXT("DelegateSignature"));
			}
			return FString(TEXT("Graph"));
		};

		if (Format == EBEPExportFormat::Text)
		{
			FString Dump;
			Dump += FString::Printf(TEXT("Blueprint: %s (%s)\n"),
				*Blueprint->GetName(),
				*Blueprint->GetPathName());

			for (UEdGraph* Graph : Graphs)
			{
				DumpGraphText(Blueprint, Graph, Dump);
				ExportGraphSnippet(Blueprint, Graph, SnippetDir);
			}

			const FString FilePath = OutDir / (SafeName + TEXT("_Flow.txt"));
			if (!FFileHelper::SaveStringToFile(Dump, *FilePath))
			{
				LogFileWriteFailure(FilePath, TEXT("Blueprint flow text"));
			}
			else
			{
				++ExportedCount;
			}
		}
		else if (Format == EBEPExportFormat::Csv)
		{
			TArray<FString> Lines;
			Lines.Add(TEXT("Blueprint,Graph,NodeTitle,NodeClass,PinName,PinCategory,PinDirection,LinkedNodeTitle,LinkedPinName"));

			auto EmitRow = [&Lines](const FString& BPName, const FString& GraphName,
				const FString& NodeTitle, const FString& NodeClass,
				const FString& PinName, const FString& PinCategory, const FString& PinDir,
				const FString& LinkedNodeTitle, const FString& LinkedPinName)
			{
				Lines.Add(FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s"),
					*ToCsvSafe(BPName),
					*ToCsvSafe(GraphName),
					*ToCsvSafe(NodeTitle),
					*ToCsvSafe(NodeClass),
					*ToCsvSafe(PinName),
					*ToCsvSafe(PinCategory),
					*ToCsvSafe(PinDir),
					*ToCsvSafe(LinkedNodeTitle),
					*ToCsvSafe(LinkedPinName)));
			};

			auto ProcessGraphCsv = [&EmitRow, &Blueprint](UEdGraph* Graph)
			{
				if (!Graph) return;

				const FString GraphName = Graph->GetName();

				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (!Node) continue;

					const FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
					const FString NodeClass = Node->GetClass()->GetName();

					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (!Pin) continue;

						const FString PinName = Pin->PinName.ToString();
						const FString PinCategory = Pin->PinType.PinCategory.ToString();
						const FString PinDir = (Pin->Direction == EGPD_Input) ? TEXT("In") : TEXT("Out");

						if (Pin->LinkedTo.Num() == 0)
						{
							EmitRow(Blueprint->GetName(), GraphName, NodeTitle, NodeClass,
								PinName, PinCategory, PinDir, FString(), FString());
						}
						else
						{
							for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
							{
								if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;

								const FString LinkedNodeTitle =
									LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
								const FString LinkedPinName = LinkedPin->PinName.ToString();

								EmitRow(Blueprint->GetName(), GraphName, NodeTitle, NodeClass,
									PinName, PinCategory, PinDir, LinkedNodeTitle, LinkedPinName);
							}
						}
					}
				}
			};

			for (UEdGraph* Graph : Graphs)
			{
				ProcessGraphCsv(Graph);
				ExportGraphSnippet(Blueprint, Graph, SnippetDir);
			}

			const FString FilePath = OutDir / (SafeName + TEXT("_Flow.csv"));
			const FString Csv = FString::Join(Lines, TEXT("\n"));

			if (!FFileHelper::SaveStringToFile(Csv, *FilePath))
			{
				LogFileWriteFailure(FilePath, TEXT("Blueprint flow CSV"));
			}
			else
			{
				++ExportedCount;
			}
		}
		else // Json
		{
			TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
			RootObj->SetStringField(TEXT("BlueprintName"), Blueprint->GetName());
			RootObj->SetStringField(TEXT("PathName"), Blueprint->GetPathName());

			TArray<TSharedPtr<FJsonValue>> GraphArray;

			auto ProcessGraphJson = [&GraphArray](UEdGraph* Graph, const FString& GraphType)
			{
				if (!Graph) return;

				TSharedRef<FJsonObject> GraphObj = MakeShared<FJsonObject>();
				GraphObj->SetStringField(TEXT("graph_name"), Graph->GetName());
				GraphObj->SetStringField(TEXT("graph_type"), GraphType);

				TArray<TSharedPtr<FJsonValue>> NodeArray;
				TArray<TSharedPtr<FJsonValue>> EdgeArray;

				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (!Node) continue;

					TSharedRef<FJsonObject> NodeObj = MakeShared<FJsonObject>();
					const FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
					const FString NodeClass = Node->GetClass()->GetName();

					NodeObj->SetStringField(TEXT("title"), NodeTitle);
					NodeObj->SetStringField(TEXT("class"), NodeClass);

					TArray<TSharedPtr<FJsonValue>> PinArray;
					TSharedRef<FJsonObject> InputsObj = MakeShared<FJsonObject>();

					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (!Pin) continue;

						TSharedRef<FJsonObject> PinObj = MakeShared<FJsonObject>();
						const FString PinName = Pin->PinName.ToString();
						const FString PinCategory = Pin->PinType.PinCategory.ToString();
						const FString PinDir = (Pin->Direction == EGPD_Input) ? TEXT("In") : TEXT("Out");
						const FString DefaultValue = FBlueprintFlowExporter::GetPinDefaultValueString(Pin);
						const FString ResolvedValue = FBlueprintFlowExporter::DescribePinResolvedValue(Pin);

						PinObj->SetStringField(TEXT("name"), PinName);
						PinObj->SetStringField(TEXT("category"), PinCategory);
						PinObj->SetStringField(TEXT("direction"), PinDir);
						PinObj->SetStringField(TEXT("default_value"), DefaultValue);
						PinObj->SetStringField(TEXT("resolved_value"), ResolvedValue);

						TArray<TSharedPtr<FJsonValue>> LinksArray;
						for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
						{
							if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;

							TSharedRef<FJsonObject> LinkObj = MakeShared<FJsonObject>();
							LinkObj->SetStringField(TEXT("NodeTitle"),
								LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
							LinkObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());

							// Also add an edge entry (from node title/pin -> linked node title/pin)
							TSharedRef<FJsonObject> EdgeObj = MakeShared<FJsonObject>();
							EdgeObj->SetStringField(TEXT("from_node_title"), NodeTitle);
							EdgeObj->SetStringField(TEXT("from_pin"), PinName);
							EdgeObj->SetStringField(TEXT("to_node_title"),
								LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
							EdgeObj->SetStringField(TEXT("to_pin"), LinkedPin->PinName.ToString());
							EdgeArray.Add(MakeShared<FJsonValueObject>(EdgeObj));

							LinksArray.Add(MakeShared<FJsonValueObject>(LinkObj));
						}

						PinObj->SetArrayField(TEXT("links"), LinksArray);
						PinArray.Add(MakeShared<FJsonValueObject>(PinObj));

						if (Pin->Direction == EGPD_Input && !ResolvedValue.IsEmpty())
						{
							InputsObj->SetStringField(PinName, ResolvedValue);
						}
					}

					NodeObj->SetArrayField(TEXT("Pins"), PinArray);
					NodeObj->SetObjectField(TEXT("inputs"), InputsObj);
					NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
				}

				GraphObj->SetArrayField(TEXT("nodes"), NodeArray);
				GraphObj->SetArrayField(TEXT("edges"), EdgeArray);
				GraphArray.Add(MakeShared<FJsonValueObject>(GraphObj));
			};

			for (UEdGraph* Graph : Graphs)
			{
				ProcessGraphJson(Graph, GetGraphTypeString(Graph));
				ExportGraphSnippet(Blueprint, Graph, SnippetDir);
			}

			RootObj->SetStringField(TEXT("asset_type"), TEXT("Blueprint"));
			RootObj->SetStringField(TEXT("asset_name"), Blueprint->GetName());
			RootObj->SetStringField(TEXT("asset_path"), Blueprint->GetPathName());
			RootObj->SetArrayField(TEXT("graphs"), GraphArray);

			const FString FilePath = OutDir / (SafeName + TEXT("_Flow.json"));

			FString JsonString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
			FJsonSerializer::Serialize(RootObj, Writer);

			if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
			{
				LogFileWriteFailure(FilePath, TEXT("Blueprint flow JSON"));
			}
			else
			{
				++ExportedCount;
			}
		}

		if (ExportedCount > 0 && (ExportedCount % GCInterval == 0))
		{
			UE_LOG(LogBEP, Log, TEXT("[BEP] Processed %d blueprints, running GC..."), ExportedCount);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
		}
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllBlueprintFlows: Exported=%d"), ExportedCount);
}

void FBlueprintFlowExporter::ExportAllInputMappingContexts(const FString& OutputDirectory, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();

	FARFilter Filter;
	Filter.ClassPaths.Add(UInputMappingContext::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(*RootPath);
	Filter.bRecursivePaths = true;

	TArray<FAssetData> IMCAssets;
	AssetRegistry.GetAssets(Filter, IMCAssets);

	const int32 TotalAssets = IMCAssets.Num();
	const int32 Limit = (MaxAssetsPerRun > 0) ? FMath::Min(MaxAssetsPerRun, TotalAssets) : TotalAssets;
	const int32 GCInterval = 50;

	const FString OutDir = OutputDirectory.IsEmpty()
		                        ? (FPaths::ProjectSavedDir() / TEXT("BEPExport/IMC"))
		                        : OutputDirectory;
	EnsureDirectory(OutDir);

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllInputMappingContexts: RootPath=%s AssetsFound=%d Limit=%d OutputDir=%s Format=%d"),
		*RootPath, IMCAssets.Num(), Limit, *OutDir, static_cast<int32>(Format));

	int32 ExportedCount = 0;

	for (int32 Index = 0; Index < Limit; ++Index)
	{
		const FAssetData& Asset = IMCAssets[Index];
		if (ShouldSkipAsset(Asset, ExcludedClassPatterns))
		{
			continue;
		}
		UInputMappingContext* IMC = Cast<UInputMappingContext>(Asset.GetAsset());
		if (!IMC)
		{
			continue;
		}

		const FString SafeName = Asset.AssetName.ToString().Replace(TEXT("/"), TEXT("_"));

		const TArray<FEnhancedActionKeyMapping>& Mappings = IMC->GetMappings();

		if (Format == EBEPExportFormat::Text)
		{
			FString Dump;
			Dump += FString::Printf(TEXT("InputMappingContext: %s (%s)\n"),
				*IMC->GetName(),
				*IMC->GetPathName());

 			for (const FEnhancedActionKeyMapping& Mapping : Mappings)
 			{
 				const FString ActionName = GetNameSafe(Mapping.Action);
 				const FString KeyName = Mapping.Key.GetDisplayName().ToString();

 				Dump += FString::Printf(TEXT("  Action=%s Key=%s\n"),
 					*ActionName,
 					*KeyName);
 			}

			const FString FilePath = OutDir / (SafeName + TEXT("_IMC.txt"));
			if (!FFileHelper::SaveStringToFile(Dump, *FilePath))
			{
				LogFileWriteFailure(FilePath, TEXT("IMC text"));
			}
			else
			{
				++ExportedCount;
			}
		}
		else if (Format == EBEPExportFormat::Csv)
		{
			TArray<FString> Lines;
			Lines.Add(TEXT("IMC,Action,Key"));

			for (const FEnhancedActionKeyMapping& Mapping : Mappings)
			{
				const FString ActionName = GetNameSafe(Mapping.Action);
				const FString KeyName = Mapping.Key.GetDisplayName().ToString();

				Lines.Add(FString::Printf(TEXT("%s,%s,%s"),
					*ToCsvSafe(IMC->GetName()),
					*ToCsvSafe(ActionName),
					*ToCsvSafe(KeyName)));
			}

 			const FString FilePath = OutDir / (SafeName + TEXT("_IMC.csv"));
			const FString Csv = FString::Join(Lines, TEXT("\n"));

			if (!FFileHelper::SaveStringToFile(Csv, *FilePath))
			{
				LogFileWriteFailure(FilePath, TEXT("IMC CSV"));
			}
			else
			{
				++ExportedCount;
			}
		}
		else // Json
		{
			TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
			RootObj->SetStringField(TEXT("asset_type"), TEXT("InputMappingContext"));
			RootObj->SetStringField(TEXT("asset_name"), IMC->GetName());
			RootObj->SetStringField(TEXT("asset_path"), IMC->GetPathName());

			TArray<TSharedPtr<FJsonValue>> MappingArray;
			for (const FEnhancedActionKeyMapping& Mapping : Mappings)
			{
				TSharedRef<FJsonObject> MappingObj = MakeShared<FJsonObject>();
				MappingObj->SetStringField(TEXT("Action"), GetNameSafe(Mapping.Action));
				MappingObj->SetStringField(TEXT("Key"), Mapping.Key.GetDisplayName().ToString());
				MappingArray.Add(MakeShared<FJsonValueObject>(MappingObj));
			}
			RootObj->SetArrayField(TEXT("Mappings"), MappingArray);

			FString JsonString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
 			FJsonSerializer::Serialize(RootObj, Writer);
 
 			const FString FilePath = OutDir / (SafeName + TEXT("_IMC.json"));
 			if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
 			{
				LogFileWriteFailure(FilePath, TEXT("IMC JSON"));
 			}
			else
			{
				++ExportedCount;
			}
		}

		if (ExportedCount > 0 && (ExportedCount % GCInterval == 0))
		{
			UE_LOG(LogBEP, Log, TEXT("[BEP] Processed %d IMCs, running GC..."), ExportedCount);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
		}
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllInputMappingContexts: Exported=%d"), ExportedCount);
}

void FBlueprintFlowExporter::ExportAllDataAssetsAndTables(const FString& OutputDirectory, const FString& RootPath, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();

	FARFilter Filter;
	Filter.PackagePaths.Add(*RootPath);
	Filter.bRecursivePaths = true;
	// We will filter by class at runtime; this pass tries to serialize as many
	// asset types as possible to JSON (DataAssets, DataTables, curves, etc.).

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	const int32 TotalAssets = Assets.Num();
	const int32 Limit = (MaxAssetsPerRun > 0) ? FMath::Min(MaxAssetsPerRun, TotalAssets) : TotalAssets;
	const int32 GCInterval = 50;

	const FString OutDir = OutputDirectory.IsEmpty()
		                        ? (FPaths::ProjectSavedDir() / TEXT("BEPExport/Data"))
		                        : OutputDirectory;
	EnsureDirectory(OutDir);

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllDataAssetsAndTables: RootPath=%s AssetsFound=%d Limit=%d OutputDir=%s"),
		*RootPath, Assets.Num(), Limit, *OutDir);

	int32 ExportedCount = 0;

 	for (int32 Index = 0; Index < Limit; ++Index)
 	{
 		const FAssetData& Asset = Assets[Index];
		if (ShouldSkipAsset(Asset, ExcludedClassPatterns))
		{
			continue;
		}
 		UObject* Obj = Asset.GetAsset();
 		if (!Obj)
 		{
 			continue;
 		}

		// Skip types that have dedicated, richer exporters elsewhere or are
		// known to produce extremely noisy / low‑value JSON dumps.
		// Blueprints and IMCs are handled by their own exporters.
		if (Obj->IsA<UBlueprint>() ||
			Obj->IsA<UInputMappingContext>())
		{
			continue;
		}

		// Levels / worlds.
		if (Obj->IsA<UWorld>() || Obj->IsA<ULevel>())
		{
			continue;
		}

		// Audio.
		if (Obj->IsA<USoundBase>())
		{
			continue;
		}

		// Mesh assets.
		if (Obj->IsA<UStaticMesh>() || Obj->IsA<USkeletalMesh>())
		{
			continue;
		}

		// Raw animation assets – keep only animation blueprints (handled above
		// as UBlueprint); skip sequences/montages here.
 		if (Obj->IsA<UAnimSequence>() || Obj->IsA<UAnimMontage>())
 		{
 			continue;
 		}

		// Assets that contain Universal Object Locator fragments can hit a hard
		// assert in engine ToString() when serialized to text/JSON. To keep BEP
		// robust, skip these and log at verbose level.
		if (StructContainsUnsupportedLocator(Obj->GetClass()))
		{
			UE_LOG(LogBEP, Verbose,
				TEXT("[BEP] Skipping %s (%s) – contains UniversalObjectLocator / ActorLocatorFragment properties"),
				*Obj->GetName(),
				*Obj->GetClass()->GetName());
			continue;
		}

  		FString Json;
  		if (!FJsonObjectConverter::UStructToJsonObjectString(
  			    Obj->GetClass(), Obj, Json, 0, 0))
  		{
			UE_LOG(LogBEP, Verbose, TEXT("[BEP] UStructToJsonObjectString failed for %s (%s)"),
				*Obj->GetName(), *Obj->GetClass()->GetName());
  			continue;
  		}

 		const FString SafeName = Asset.AssetName.ToString().Replace(TEXT("/"), TEXT("_"));
 		const FString FilePath = OutDir / (SafeName + TEXT(".json"));
 
 		if (!FFileHelper::SaveStringToFile(Json, *FilePath))
 		{
			LogFileWriteFailure(FilePath, TEXT("DataAsset/DataTable JSON"));
 		}
		else
		{
			++ExportedCount;
		}

		if (ExportedCount > 0 && (ExportedCount % GCInterval == 0))
		{
			UE_LOG(LogBEP, Log, TEXT("[BEP] Processed %d data assets, running GC..."), ExportedCount);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
		}
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllDataAssetsAndTables: Exported=%d"), ExportedCount);
}

void FBlueprintFlowExporter::ExportAllStructSchemas(const FString& OutputFilePath)
{
	FString OutPath = OutputFilePath;
	if (OutPath.IsEmpty())
	{
		OutPath = FPaths::ProjectSavedDir() / TEXT("BEPExport/StructSchemas.txt");
	}

	FString Dump;

	ForEachObjectOfClass(UScriptStruct::StaticClass(), [&Dump](UObject* Obj)
	{
		UScriptStruct* Struct = Cast<UScriptStruct>(Obj);
		if (!Struct)
		{
			return true;
		}

		// Skip non-project structs if you want only game ones.
		const FString PackageName = Struct->GetOutermost()->GetName();
		if (!PackageName.StartsWith(TEXT("/Game")))
		{
			return true;
		}

		Dump += FString::Printf(TEXT("Struct: %s (%s)\n"),
			*Struct->GetName(),
			*Struct->GetPathName());

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			Dump += FString::Printf(TEXT("  %s %s\n"),
				*Prop->GetClass()->GetName(),
				*Prop->GetName());
		}

		Dump += TEXT("\n");
		return true;
 	});
 
 	if (!FFileHelper::SaveStringToFile(Dump, *OutPath))
 	{
		LogFileWriteFailure(OutPath, TEXT("Struct schema text"));
 	}
	else
	{
		UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAllStructSchemas: Wrote schema file to %s"), *OutPath);
	}
}

void FBlueprintFlowExporter::ExportAll(const FString& OutputRoot, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun)
{
	// Hard‑coded export base so we never end up under the engine install or
	// an unexpected working directory. This keeps all BEP exports in a single,
	// predictable location the user can clean or diff easily.
	const FString ExportBaseDir = TEXT("C:/BEP_EXPORTS");
	EnsureDirectory(ExportBaseDir);

	// Optional sub‑folder under the base, coming from the console command.
	FString CleanOutputRoot = OutputRoot;
	// Strip any quotes that might have come from the console argument.
	CleanOutputRoot.ReplaceInline(TEXT("\""), TEXT(""));
	CleanOutputRoot.TrimStartAndEndInline();

	const FString RootDir = CleanOutputRoot.IsEmpty()
		                         ? ExportBaseDir
		                         : FPaths::Combine(ExportBaseDir, CleanOutputRoot);

	EnsureDirectory(RootDir);

	const FString AssetRoot = RootPath.IsEmpty() ? TEXT("/Game") : RootPath;

	UE_LOG(LogBEP, Log, TEXT("[BEP] ExportAll: RootDir=%s AssetRoot=%s Format=%d"),
		*RootDir, *AssetRoot, static_cast<int32>(Format));

	ExportAllBlueprintFlows(RootDir / TEXT("BlueprintFlows"), AssetRoot, Format, ExcludedClassPatterns, MaxAssetsPerRun);
	ExportAllInputMappingContexts(RootDir / TEXT("IMC"), AssetRoot, Format, ExcludedClassPatterns, MaxAssetsPerRun);
	ExportAllDataAssetsAndTables(RootDir / TEXT("Data"), AssetRoot, ExcludedClassPatterns, MaxAssetsPerRun);
	ExportAllStructSchemas(RootDir / TEXT("StructSchemas.txt"));
}

EBEPExportFormat FBlueprintFlowExporter::ParseFormat(const FString& InFormatString)
{
	const FString Lower = InFormatString.ToLower();
	if (Lower == TEXT("json"))
	{
		return EBEPExportFormat::Json;
	}
	if (Lower == TEXT("csv"))
	{
		return EBEPExportFormat::Csv;
	}
	return EBEPExportFormat::Text;
}
