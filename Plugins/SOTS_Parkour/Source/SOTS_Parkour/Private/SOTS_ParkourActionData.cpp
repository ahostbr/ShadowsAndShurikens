// SOTS_ParkourActionData.cpp
// SPINE V3_03 – Parkour action data helpers and JSON import.

#include "SOTS_ParkourActionData.h"

#include "Animation/AnimMontage.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
ESOTS_ParkourAction ActionFromName(const FString& Name)
{
	const FString Lower = Name.ToLower();
	if (Lower.Contains(TEXT("mantle"))) return ESOTS_ParkourAction::Mantle;
	if (Lower.Contains(TEXT("lowmantle"))) return ESOTS_ParkourAction::LowMantle;
	if (Lower.Contains(TEXT("distance"))) return ESOTS_ParkourAction::DistanceMantle;
	if (Lower.Contains(TEXT("thinvault"))) return ESOTS_ParkourAction::ThinVault;
	if (Lower.Contains(TEXT("highvault"))) return ESOTS_ParkourAction::HighVault;
	if (Lower.Contains(TEXT("vaulttobraced"))) return ESOTS_ParkourAction::VaultToBraced;
	if (Lower.Contains(TEXT("vault"))) return ESOTS_ParkourAction::Vault;
	if (Lower.Contains(TEXT("tic"))) return ESOTS_ParkourAction::TicTac;
	if (Lower.Contains(TEXT("ledge"))) return ESOTS_ParkourAction::LedgeMove;
	if (Lower.Contains(TEXT("climbingup"))) return ESOTS_ParkourAction::LedgeMove;
	if (Lower.Contains(TEXT("climb"))) return ESOTS_ParkourAction::LedgeMove;
	if (Lower.Contains(TEXT("backhop"))) return ESOTS_ParkourAction::BackHop;
	if (Lower.Contains(TEXT("predict"))) return ESOTS_ParkourAction::PredictJump;
	if (Lower.Contains(TEXT("airhang"))) return ESOTS_ParkourAction::AirHang;
	if (Lower.Contains(TEXT("drop"))) return ESOTS_ParkourAction::Drop;
	return ESOTS_ParkourAction::None;
}

float GetNumberFieldSafe(const TSharedPtr<FJsonObject>& Obj, const FString& Field, float DefaultValue = 0.0f)
{
	double Out = DefaultValue;
	Obj->TryGetNumberField(Field, Out);
	return static_cast<float>(Out);
}

bool GetBoolFieldSafe(const TSharedPtr<FJsonObject>& Obj, const FString& Field, bool DefaultValue = false)
{
	bool Out = DefaultValue;
	Obj->TryGetBoolField(Field, Out);
	return Out;
}

FName GetNameFieldSafe(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
{
	FString Str;
	return Obj->TryGetStringField(Field, Str) ? FName(*Str) : NAME_None;
}

FGameplayTag GetTagFieldSafe(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
{
	const TSharedPtr<FJsonObject>* TagObj;
	if (Obj->TryGetObjectField(Field, TagObj))
	{
		FString TagName;
		if ((*TagObj)->TryGetStringField(TEXT("TagName"), TagName))
		{
			return FGameplayTag::RequestGameplayTag(*TagName, false);
		}
	}
	return FGameplayTag();
}

FString NormalizeMontagePath(const FString& InPath)
{
	// We accept inputs like:
	//   /Script/Engine.AnimMontage'/ParkourSystem/.../MantleUEFN_Montage.MantleUEFN_Montage'
	//   /ParkourSystem/.../MantleUEFN_Montage.MantleUEFN_Montage
	// We return a plain soft object path without quotes or class prefix:
	//   /Game/ParkourSystem/.../MantleUEFN_Montage.MantleUEFN_Montage

	FString Path = InPath.TrimStartAndEnd();

	int32 FirstQuote = INDEX_NONE;
	int32 LastQuote = INDEX_NONE;
	if (Path.FindChar(TEXT('\''), FirstQuote) && Path.FindLastChar(TEXT('\''), LastQuote) && LastQuote > FirstQuote)
	{
		Path = Path.Mid(FirstQuote + 1, LastQuote - FirstQuote - 1);
	}

	// Remap the legacy UEFN root folder to the actual project path used by CGF imports.
	const FString GameParkourPrefix = TEXT("/Game/ParkourSystem/");
	const FString ParkourPrefix = TEXT("/ParkourSystem/");
	const FString GameCgfPrefix = TEXT("/Game/CGF_Parkour/");

	if (Path.StartsWith(GameParkourPrefix))
	{
		Path = GameCgfPrefix + Path.Mid(GameParkourPrefix.Len());
	}
	else if (Path.StartsWith(ParkourPrefix))
	{
		Path = GameCgfPrefix + Path.Mid(ParkourPrefix.Len());
	}

	if (!Path.StartsWith(TEXT("/Game")) && Path.StartsWith(TEXT("/")))
	{
		Path = TEXT("/Game") + Path;
	}

	return Path;
}
} // namespace

USOTS_ParkourActionSet::USOTS_ParkourActionSet()
{
	DefaultUEFNJsonPath = TEXT("E:/SAS/ShadowsAndShurikens/Content/UEFNParkourActionVariablesDT_CGF.json");
}

bool USOTS_ParkourActionSet::LoadFromJsonFile(const FString& FilePath)
{
	Modify();

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("USOTS_ParkourActionSet: Failed to load JSON file %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("USOTS_ParkourActionSet: Failed to parse JSON %s"), *FilePath);
		return false;
	}

	if (RootValue->Type != EJson::Array)
	{
		UE_LOG(LogTemp, Warning, TEXT("USOTS_ParkourActionSet: Expected array at root %s"), *FilePath);
		return false;
	}

	Actions.Reset();

	const TArray<TSharedPtr<FJsonValue>>* ArrayPtr;
	if (!RootValue->TryGetArray(ArrayPtr))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *ArrayPtr)
	{
		if (!Value.IsValid() || Value->Type != EJson::Object)
		{
			continue;
		}

		const TSharedPtr<FJsonObject> RowObj = Value->AsObject();
		FSOTS_ParkourActionDefinition Def;

		FString NameStr;
		if (RowObj->TryGetStringField(TEXT("Name"), NameStr))
		{
			Def.RowName = FName(*NameStr);
			Def.ActionTag = FGameplayTag::RequestGameplayTag(*NameStr, false);
			Def.ActionType = ActionFromName(NameStr);
		}

		FString MontagePath;
		if (RowObj->TryGetStringField(TEXT("Parkour Montage"), MontagePath))
		{
			// Prefer the plain /Game/... path so the editor resolves correctly.
			const FString NormalizedPath = NormalizeMontagePath(MontagePath);
			FSoftObjectPath SoftPath(NormalizedPath);
			Def.ParkourMontage = TSoftObjectPtr<UAnimMontage>(SoftPath);

			if (!FPackageName::DoesPackageExist(SoftPath.GetLongPackageName()))
			{
				UE_LOG(LogTemp, Warning, TEXT("USOTS_ParkourActionSet: Montage package missing or wrong path: %s (from %s)"), *NormalizedPath, *MontagePath);
			}
		}

		Def.PlayRate = GetNumberFieldSafe(RowObj, TEXT("Play Rate"), 1.0f);
		Def.StartingPosition = GetNumberFieldSafe(RowObj, TEXT("Starting Position"), 0.0f);
		Def.FallingPosition = GetNumberFieldSafe(RowObj, TEXT("Falling Position"), 0.0f);
		Def.InStateTag = GetTagFieldSafe(RowObj, TEXT("InState"));
		Def.OutStateTag = GetTagFieldSafe(RowObj, TEXT("OutState"));

		const TArray<TSharedPtr<FJsonValue>>* WarpArray;
		if (RowObj->TryGetArrayField(TEXT("MotionWarpingSettings"), WarpArray))
		{
			for (const TSharedPtr<FJsonValue>& WarpValue : *WarpArray)
			{
				if (!WarpValue.IsValid() || WarpValue->Type != EJson::Object)
				{
					continue;
				}

				const TSharedPtr<FJsonObject> WarpObj = WarpValue->AsObject();
				FSOTS_ParkourWarpSettings Warp;

				Warp.StartTime = GetNumberFieldSafe(WarpObj, TEXT("Start Time"));
				Warp.EndTime   = GetNumberFieldSafe(WarpObj, TEXT("End Time"));
				Warp.WarpTargetName = GetNameFieldSafe(WarpObj, TEXT("Warp Target Name"));
				Warp.WarpPointAnimProvider = GetNameFieldSafe(WarpObj, TEXT("Warp Point Anim Provider"));
				Warp.WarpPointAnimBoneName = GetNameFieldSafe(WarpObj, TEXT("Warp Point Anim Bone Name"));
				Warp.bWarpTranslation = GetBoolFieldSafe(WarpObj, TEXT("Warp Translation"), true);
				Warp.bIgnoreZAxis = GetBoolFieldSafe(WarpObj, TEXT("Ignore Z Axis"), false);
				Warp.bWarpRotation = GetBoolFieldSafe(WarpObj, TEXT("Warp Rotation"), true);
				Warp.RotationType = GetNameFieldSafe(WarpObj, TEXT("Rotation Type"));
				Warp.WarpTransformType = GetTagFieldSafe(WarpObj, TEXT("Warp Transform Type"));
				Warp.XOffset = GetNumberFieldSafe(WarpObj, TEXT("X Offset"));
				Warp.ZOffset = GetNumberFieldSafe(WarpObj, TEXT("Z Offset"));
				Warp.bReversedRotation = GetBoolFieldSafe(WarpObj, TEXT("Reversed Rotation"), false);
				Warp.bAdjustForCharacterHeight = GetBoolFieldSafe(WarpObj, TEXT("Character Dif Height Adjustmen"), false);

				Def.WarpSettings.Add(Warp);
			}
		}

		Actions.Add(Def);
	}

	UE_LOG(LogTemp, Log, TEXT("USOTS_ParkourActionSet: Loaded %d actions from %s"), Actions.Num(), *FilePath);
	return Actions.Num() > 0;
}

bool USOTS_ParkourActionSet::LoadFromDefaultUEFNJson()
{
#if WITH_EDITOR
	const FString ResolvedPath = FPaths::ConvertRelativePathToFull(DefaultUEFNJsonPath);
	if (!FPaths::FileExists(ResolvedPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("USOTS_ParkourActionSet: Default UEFN JSON path does not exist: %s"), *ResolvedPath);
		return false;
	}

	return LoadFromJsonFile(ResolvedPath);
#else
	return false;
#endif
}

#if WITH_EDITOR
void USOTS_ParkourActionSet::PreSave(FObjectPreSaveContext SaveContext)
{
	if (bAutoImportOnSave && !DefaultUEFNJsonPath.IsEmpty())
	{
		LoadFromDefaultUEFNJson();
	}

	Super::PreSave(SaveContext);
}

void USOTS_ParkourActionSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(USOTS_ParkourActionSet, bForceImport))
	{
		if (bForceImport)
		{
			LoadFromDefaultUEFNJson();
			bForceImport = false;
		}
	}
}
#endif
