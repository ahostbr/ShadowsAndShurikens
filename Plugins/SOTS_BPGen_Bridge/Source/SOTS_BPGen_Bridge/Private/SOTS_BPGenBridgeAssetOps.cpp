#include "SOTS_BPGenBridgeAssetOps.h"

#include "AssetRegistry/AssetRegistryTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "EditorAssetLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"

#include "AssetToolsModule.h"
#include "AssetRenameData.h"
#include "AutomatedAssetImportData.h"
#include "IAssetTools.h"

#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

#include "Editor/EditorEngine.h"
#include "FileHelpers.h"

namespace
{
	static FString NormalizeLongPackagePath(const FString& InPath)
	{
		FString Path = InPath;
		Path.TrimStartAndEndInline();
		if (Path.IsEmpty())
		{
			return FString();
		}

		if (Path.StartsWith(TEXT("/Game")) || Path.StartsWith(TEXT("/Engine")) || Path.StartsWith(TEXT("/Script")))
		{
			return Path;
		}

		if (Path.StartsWith(TEXT("/")))
		{
			return FString(TEXT("/Game")) + Path;
		}

		return FString(TEXT("/Game/")) + Path;
	}

	static FString NormalizeAssetObjectPath(const FString& InAssetPath)
	{
		FString Path = NormalizeLongPackagePath(InAssetPath);
		if (Path.IsEmpty())
		{
			return FString();
		}
		if (Path.Contains(TEXT(".")))
		{
			return Path;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
		if (AssetName.IsEmpty())
		{
			return Path;
		}

		return FString::Printf(TEXT("%s.%s"), *Path, *AssetName);
	}

	static FSOTS_BPGenBridgeAssetOpResult MakeError(const FString& ErrorCode, const FString& ErrorMessage)
	{
		FSOTS_BPGenBridgeAssetOpResult R;
		R.bOk = false;
		R.ErrorCode = ErrorCode;
		R.Errors.Add(ErrorMessage);
		R.Result = MakeShared<FJsonObject>();
		return R;
	}

	static void AddStringArray(TSharedPtr<FJsonObject> Obj, const FString& Field, const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		Arr.Reserve(Values.Num());
		for (const FString& V : Values)
		{
			Arr.Add(MakeShared<FJsonValueString>(V));
		}
		Obj->SetArrayField(Field, Arr);
	}

	static FString NormalizeContentFolderPath(const FString& InPath)
	{
		FString Path = NormalizeLongPackagePath(InPath);
		if (Path.IsEmpty())
		{
			return FString(TEXT("/Game"));
		}
		if (Path.Contains(TEXT(".")))
		{
			// Likely an object path; best-effort strip to package path.
			return FPackageName::ObjectPathToPackageName(Path);
		}
		return Path;
	}

	static bool EnsureDirectoryExistsOnDisk(const FString& Directory)
	{
		return IFileManager::Get().MakeDirectory(*Directory, true);
	}

	static FString MakeUniqueTempFilePath(const FString& Folder, const FString& BaseName, const FString& Extension)
	{
		const FString SafeExt = Extension.StartsWith(TEXT(".")) ? Extension.Mid(1) : Extension;
		const FString Guid = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		return FPaths::Combine(Folder, FString::Printf(TEXT("%s_%s.%s"), *BaseName, *Guid, *SafeExt));
	}

	static bool ParseMaxSize(const TSharedPtr<FJsonObject>& Params, int32& OutW, int32& OutH)
	{
		OutW = 0;
		OutH = 0;
		if (!Params.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* ArrPtr = nullptr;
		if (!Params->TryGetArrayField(TEXT("max_size"), ArrPtr) || !ArrPtr || ArrPtr->Num() < 2)
		{
			return false;
		}

		OutW = static_cast<int32>((*ArrPtr)[0]->AsNumber());
		OutH = static_cast<int32>((*ArrPtr)[1]->AsNumber());
		return true;
	}

	static FSOTS_BPGenBridgeAssetOpResult SaveBinaryFile(const FString& FilePath, const TArray64<uint8>& Bytes)
	{
		TArray<uint8> Narrow;
		Narrow.Append(Bytes.GetData(), static_cast<int32>(Bytes.Num()));
		if (!FFileHelper::SaveArrayToFile(Narrow, *FilePath))
		{
			return MakeError(TEXT("ASSET_EXPORT_FAILED"), FString::Printf(TEXT("Failed to write file: %s"), *FilePath));
		}
		FSOTS_BPGenBridgeAssetOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		return R;
	}
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::Search(const TSharedPtr<FJsonObject>& Params)
{
	FString SearchTerm;
	FString AssetType;
	FString RootPath = TEXT("/Game");
	bool bCaseSensitive = false;
	bool bIncludeEngineContent = false;
	int32 MaxResults = 100;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("search_term"), SearchTerm);
		Params->TryGetStringField(TEXT("asset_type"), AssetType);
		Params->TryGetStringField(TEXT("path"), RootPath);
		Params->TryGetBoolField(TEXT("case_sensitive"), bCaseSensitive);
		Params->TryGetBoolField(TEXT("include_engine_content"), bIncludeEngineContent);
		Params->TryGetNumberField(TEXT("max_results"), MaxResults);
	}

	RootPath = NormalizeContentFolderPath(RootPath);
	SearchTerm.TrimStartAndEndInline();
	AssetType.TrimStartAndEndInline();

	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = Arm.Get();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(*RootPath);
	if (bIncludeEngineContent && RootPath.StartsWith(TEXT("/Game")))
	{
		Filter.PackagePaths.Add(TEXT("/Engine"));
	}

	auto AddClassFilter = [&Filter](const FTopLevelAssetPath& ClassPath)
	{
		Filter.ClassPaths.Add(ClassPath);
	};

	const FString AssetTypeLower = AssetType.ToLower();
	if (AssetTypeLower == TEXT("texture2d"))
	{
		AddClassFilter(UTexture2D::StaticClass()->GetClassPathName());
	}
	else if (AssetTypeLower == TEXT("material"))
	{
		AddClassFilter(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Material")));
	}
	else if (AssetTypeLower == TEXT("blueprint"))
	{
		AddClassFilter(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
	}
	else if (AssetTypeLower == TEXT("widget"))
	{
		AddClassFilter(FTopLevelAssetPath(TEXT("/Script/UMGEditor"), TEXT("WidgetBlueprint")));
	}

	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);

	const ESearchCase::Type CaseMode = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

	TArray<TSharedPtr<FJsonValue>> OutAssets;
	OutAssets.Reserve(FMath::Min(Assets.Num(), MaxResults));

	int32 Count = 0;
	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		if (!SearchTerm.IsEmpty() && !Name.Contains(SearchTerm, CaseMode))
		{
			continue;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("package_path"), Asset.PackageName.ToString());
		Obj->SetStringField(TEXT("object_path"), Asset.GetObjectPathString());
		Obj->SetStringField(TEXT("class"), Asset.AssetClassPath.ToString());
		OutAssets.Add(MakeShared<FJsonValueObject>(Obj));
		Count++;
		if (MaxResults > 0 && Count >= MaxResults)
		{
			break;
		}
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetArrayField(TEXT("assets"), OutAssets);
	R.Result->SetNumberField(TEXT("count"), Count);
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::OpenInEditor(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	bool bForceOpen = false;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetBoolField(TEXT("force_open"), bForceOpen);
	}

	if (AssetPath.TrimStartAndEnd().IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("open_in_editor requires asset_path"));
	}

	const FString ObjectPath = NormalizeAssetObjectPath(AssetPath);
	UObject* Asset = UEditorAssetLibrary::LoadAsset(ObjectPath);
	if (!Asset)
	{
		return MakeError(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Failed to load asset: %s"), *ObjectPath));
	}

	if (!GEditor)
	{
		return MakeError(TEXT("INTERNAL_ERROR"), TEXT("GEditor not available"));
	}

	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!Subsystem)
	{
		return MakeError(TEXT("INTERNAL_ERROR"), TEXT("AssetEditorSubsystem not available"));
	}

	if (bForceOpen)
	{
		Subsystem->CloseAllEditorsForAsset(Asset);
	}

	Subsystem->OpenEditorForAsset(Asset);

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), ObjectPath);
	R.Result->SetBoolField(TEXT("opened"), true);
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::Duplicate(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	FString DestinationPath;
	FString NewName;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("destination_path"), DestinationPath);
		Params->TryGetStringField(TEXT("new_name"), NewName);
	}

	if (AssetPath.TrimStartAndEnd().IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("duplicate requires asset_path"));
	}
	if (DestinationPath.TrimStartAndEnd().IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("duplicate requires destination_path"));
	}

	const FString SourceObjectPath = NormalizeAssetObjectPath(AssetPath);
	UObject* Source = UEditorAssetLibrary::LoadAsset(SourceObjectPath);
	if (!Source)
	{
		return MakeError(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Failed to load asset: %s"), *SourceObjectPath));
	}

	DestinationPath = NormalizeContentFolderPath(DestinationPath);
	if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationPath))
	{
		UEditorAssetLibrary::MakeDirectory(DestinationPath);
	}

	if (NewName.TrimStartAndEnd().IsEmpty())
	{
		NewName = Source->GetName();
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = AssetToolsModule.Get();

	UObject* Duplicated = AssetTools.DuplicateAsset(NewName, DestinationPath, Source);
	if (!Duplicated)
	{
		return MakeError(TEXT("OPERATION_FAILED"), TEXT("DuplicateAsset failed"));
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("source_asset_path"), SourceObjectPath);
	R.Result->SetStringField(TEXT("new_asset_path"), Duplicated->GetPathName());
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::Delete(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	bool bForceDelete = false;
	bool bShowConfirmation = true;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetBoolField(TEXT("force_delete"), bForceDelete);
		Params->TryGetBoolField(TEXT("show_confirmation"), bShowConfirmation);
	}

	if (AssetPath.TrimStartAndEnd().IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("delete requires asset_path"));
	}

	const FString ObjectPath = NormalizeAssetObjectPath(AssetPath);
	if (ObjectPath.StartsWith(TEXT("/Engine/")))
	{
		return MakeError(TEXT("ASSET_READ_ONLY"), FString::Printf(TEXT("Cannot delete engine content: %s"), *ObjectPath));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(ObjectPath))
	{
		return MakeError(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Asset not found: %s"), *ObjectPath));
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = Arm.Get();

	TArray<FName> Referencers;
	Registry.GetReferencers(*PackageName, Referencers, UE::AssetRegistry::EDependencyCategory::Package);
	Referencers.Remove(*PackageName);

	if (Referencers.Num() > 0 && !bForceDelete)
	{
		TArray<FString> Refs;
		Refs.Reserve(Referencers.Num());
		for (const FName& Ref : Referencers)
		{
			Refs.Add(Ref.ToString());
		}
		FSOTS_BPGenBridgeAssetOpResult R = MakeError(TEXT("ASSET_IN_USE"), FString::Printf(TEXT("Asset has %d referencer(s). Use force_delete=true to override."), Referencers.Num()));
		AddStringArray(R.Result, TEXT("referencers"), Refs);
		return R;
	}

	if (bShowConfirmation)
	{
		const FString Msg = FString::Printf(TEXT("Delete asset '%s'?"), *ObjectPath);
		const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Msg));
		if (Choice != EAppReturnType::Yes)
		{
			return MakeError(TEXT("OPERATION_CANCELLED"), TEXT("User cancelled deletion"));
		}
	}

	const bool bDeleted = UEditorAssetLibrary::DeleteAsset(ObjectPath);
	if (!bDeleted)
	{
		return MakeError(TEXT("ASSET_DELETE_FAILED"), FString::Printf(TEXT("Failed to delete asset: %s"), *ObjectPath));
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), ObjectPath);
	R.Result->SetBoolField(TEXT("deleted"), true);
	R.Result->SetNumberField(TEXT("referencer_count"), Referencers.Num());
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::ImportTexture(const TSharedPtr<FJsonObject>& Params)
{
	FString FilePath;
	FString DestinationPath = TEXT("/Game/Textures/Imported");
	FString TextureName;
	FString CompressionSettings = TEXT("TC_Default");
	bool bGenerateMipmaps = true;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("file_path"), FilePath);
		Params->TryGetStringField(TEXT("destination_path"), DestinationPath);
		Params->TryGetStringField(TEXT("texture_name"), TextureName);
		Params->TryGetStringField(TEXT("compression_settings"), CompressionSettings);
		Params->TryGetBoolField(TEXT("generate_mipmaps"), bGenerateMipmaps);
	}

	FilePath.TrimStartAndEndInline();
	if (FilePath.IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("import_texture requires file_path"));
	}
	if (!FPaths::FileExists(FilePath))
	{
		return MakeError(TEXT("FILE_NOT_FOUND"), FString::Printf(TEXT("File not found: %s"), *FilePath));
	}

	DestinationPath = NormalizeContentFolderPath(DestinationPath);
	if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationPath))
	{
		UEditorAssetLibrary::MakeDirectory(DestinationPath);
	}

	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
	ImportData->bReplaceExisting = true;
	ImportData->DestinationPath = DestinationPath;
	ImportData->Filenames.Add(FilePath);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = AssetToolsModule.Get();

	TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);
	if (ImportedAssets.Num() == 0)
	{
		return MakeError(TEXT("TEXTURE_IMPORT_FAILED"), TEXT("No assets imported"));
	}

	UObject* Imported = ImportedAssets[0];
	if (!TextureName.TrimStartAndEnd().IsEmpty())
	{
		TArray<FAssetRenameData> Renames;
		Renames.Add(FAssetRenameData(Imported, DestinationPath, TextureName));
		AssetTools.RenameAssets(Renames);
	}

	UTexture2D* Texture = Cast<UTexture2D>(Imported);
	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), Imported->GetPathName());
	R.Result->SetStringField(TEXT("destination_path"), DestinationPath);
	R.Result->SetStringField(TEXT("source_file"), FilePath);
	R.Result->SetStringField(TEXT("asset_class"), Imported->GetClass() ? Imported->GetClass()->GetName() : FString());

	if (Texture)
	{
		if (UEnum* Enum = StaticEnum<TextureCompressionSettings>())
		{
			const int64 EnumValue = Enum->GetValueByNameString(CompressionSettings);
			if (EnumValue != INDEX_NONE)
			{
				Texture->CompressionSettings = static_cast<TextureCompressionSettings>(EnumValue);
			}
			else
			{
				R.Warnings.Add(FString::Printf(TEXT("Unknown compression_settings '%s'"), *CompressionSettings));
			}
		}
		Texture->MipGenSettings = bGenerateMipmaps ? TMGS_FromTextureGroup : TMGS_NoMipmaps;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
	}

	UEditorAssetLibrary::SaveAsset(Imported->GetPathName(), true);

	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::ExportTexture(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	FString ExportFormat = TEXT("PNG");
	FString TempFolder;

	int32 MaxW = 0;
	int32 MaxH = 0;
	ParseMaxSize(Params, MaxW, MaxH);

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("export_format"), ExportFormat);
		Params->TryGetStringField(TEXT("temp_folder"), TempFolder);
	}

	AssetPath.TrimStartAndEndInline();
	if (AssetPath.IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("export_texture requires asset_path"));
	}

	const FString ObjectPath = NormalizeAssetObjectPath(AssetPath);
	UObject* Asset = UEditorAssetLibrary::LoadAsset(ObjectPath);
	UTexture2D* Texture = Cast<UTexture2D>(Asset);
	if (!Texture)
	{
		return MakeError(TEXT("ASSET_TYPE_INCORRECT"), FString::Printf(TEXT("Asset is not a Texture2D: %s"), *ObjectPath));
	}

	if (MaxW > 0 || MaxH > 0)
	{
		// Continue, but export uses source resolution for now.
	}

	if (TempFolder.TrimStartAndEnd().IsEmpty())
	{
		TempFolder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Temp"), TEXT("TextureExports"));
	}

	EnsureDirectoryExistsOnDisk(TempFolder);

	const FString ExtLower = ExportFormat.TrimStartAndEnd().ToLower();
	EImageFormat ImageFormat = EImageFormat::PNG;
	FString Ext = TEXT("png");
	if (ExtLower == TEXT("tga"))
	{
		ImageFormat = EImageFormat::TGA;
		Ext = TEXT("tga");
	}

	TArray64<uint8> Raw;
	Texture->Source.GetMipData(Raw, 0);
	const int32 Width = Texture->Source.GetSizeX();
	const int32 Height = Texture->Source.GetSizeY();
	const ETextureSourceFormat SrcFmt = Texture->Source.GetFormat();

	ERGBFormat RGBFmt = ERGBFormat::BGRA;
	if (SrcFmt == TSF_RGBA8)
	{
		RGBFmt = ERGBFormat::RGBA;
	}
	else if (SrcFmt == TSF_BGRA8)
	{
		RGBFmt = ERGBFormat::BGRA;
	}
	else
	{
		return MakeError(TEXT("TEXTURE_FORMAT_UNSUPPORTED"), TEXT("Texture source format unsupported for export"));
	}

	IImageWrapperModule& WrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = WrapperModule.CreateImageWrapper(ImageFormat);
	if (!Wrapper.IsValid() || !Wrapper->SetRaw(Raw.GetData(), Raw.Num(), Width, Height, RGBFmt, 8))
	{
		return MakeError(TEXT("ASSET_EXPORT_FAILED"), TEXT("Failed to encode texture"));
	}

	const TArray64<uint8>& Compressed = Wrapper->GetCompressed();
	const FString FilePath = MakeUniqueTempFilePath(TempFolder, Texture->GetName(), Ext);
	FSOTS_BPGenBridgeAssetOpResult SaveRes = SaveBinaryFile(FilePath, Compressed);
	if (!SaveRes.bOk)
	{
		return SaveRes;
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), ObjectPath);
	R.Result->SetStringField(TEXT("temp_file_path"), FilePath);
	R.Result->SetStringField(TEXT("export_format"), ExportFormat);

	TArray<TSharedPtr<FJsonValue>> SizeArr;
	SizeArr.Add(MakeShared<FJsonValueNumber>(Width));
	SizeArr.Add(MakeShared<FJsonValueNumber>(Height));
	R.Result->SetArrayField(TEXT("exported_size"), SizeArr);
	R.Result->SetNumberField(TEXT("file_size"), static_cast<double>(Compressed.Num()));
	R.Result->SetBoolField(TEXT("cleanup_required"), true);
	if (MaxW > 0 || MaxH > 0)
	{
		R.Warnings.Add(TEXT("export_texture max_size requested but export used source resolution."));
	}
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::Save(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
	}
	AssetPath.TrimStartAndEndInline();
	if (AssetPath.IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("save requires asset_path"));
	}

	const FString ObjectPath = NormalizeAssetObjectPath(AssetPath);
	if (!UEditorAssetLibrary::DoesAssetExist(ObjectPath))
	{
		return MakeError(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Asset not found: %s"), *ObjectPath));
	}

	const bool bSaved = UEditorAssetLibrary::SaveAsset(ObjectPath, true);
	if (!bSaved)
	{
		return MakeError(TEXT("OPERATION_FAILED"), FString::Printf(TEXT("Failed to save asset: %s"), *ObjectPath));
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), ObjectPath);
	R.Result->SetBoolField(TEXT("saved"), true);
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::SaveAll(const TSharedPtr<FJsonObject>& Params)
{
	bool bPromptUser = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("prompt_user"), bPromptUser);
	}

	const bool bSuccess = FEditorFileUtils::SaveDirtyPackages(
		/*bPromptUserToSave=*/bPromptUser,
		/*bSaveMapPackages=*/true,
		/*bSaveContentPackages=*/true,
		/*bFastSave=*/false,
		/*bNotifyNoPackagesSaved=*/!bPromptUser,
		/*bCanBeDeclined=*/bPromptUser);

	if (!bSuccess)
	{
		return MakeError(TEXT("OPERATION_FAILED"), TEXT("SaveDirtyPackages failed"));
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetBoolField(TEXT("saved_all"), true);
	return R;
}

FSOTS_BPGenBridgeAssetOpResult SOTS_BPGenBridgeAssetOps::ListReferences(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	bool bIncludeDependencies = false;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetBoolField(TEXT("include_dependencies"), bIncludeDependencies);
	}

	AssetPath.TrimStartAndEndInline();
	if (AssetPath.IsEmpty())
	{
		return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("list_references requires asset_path"));
	}

	const FString ObjectPath = NormalizeAssetObjectPath(AssetPath);
	if (!UEditorAssetLibrary::DoesAssetExist(ObjectPath))
	{
		return MakeError(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Asset not found: %s"), *ObjectPath));
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = Arm.Get();

	TArray<FName> Referencers;
	Registry.GetReferencers(*PackageName, Referencers, UE::AssetRegistry::EDependencyCategory::Package);
	Referencers.Remove(*PackageName);

	TArray<FString> RefStrings;
	RefStrings.Reserve(Referencers.Num());
	for (const FName& Ref : Referencers)
	{
		RefStrings.Add(Ref.ToString());
	}

	FSOTS_BPGenBridgeAssetOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("asset_path"), ObjectPath);
	AddStringArray(R.Result, TEXT("referencers"), RefStrings);
	R.Result->SetNumberField(TEXT("referencer_count"), Referencers.Num());

	if (bIncludeDependencies)
	{
		TArray<FName> Deps;
		Registry.GetDependencies(*PackageName, Deps, UE::AssetRegistry::EDependencyCategory::Package);
		Deps.Remove(*PackageName);
		TArray<FString> DepStrings;
		DepStrings.Reserve(Deps.Num());
		for (const FName& Dep : Deps)
		{
			DepStrings.Add(Dep.ToString());
		}
		AddStringArray(R.Result, TEXT("dependencies"), DepStrings);
		R.Result->SetNumberField(TEXT("dependency_count"), Deps.Num());
	}

	return R;
}
