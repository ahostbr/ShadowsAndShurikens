#pragma once

#include "CoreMinimal.h"

#include "Misc/PackageName.h"

#include "UObject/UnrealType.h"

namespace SOTS_BPGenBridgePrivate
{
	inline FString NormalizeLongPackagePath(const FString& InPath)
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

	inline FString NormalizeAssetObjectPath(const FString& InAssetPath)
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

	inline bool SetObjectPropertyByImportText(UObject* Target, const FString& PropertyName, const FString& ImportText, FString& OutError)
	{
		OutError.Reset();
		if (!Target)
		{
			OutError = TEXT("Target is null");
			return false;
		}

		FString PName = PropertyName;
		PName.TrimStartAndEndInline();
		if (PName.IsEmpty())
		{
			OutError = TEXT("property_name is empty");
			return false;
		}

		FProperty* Prop = FindFProperty<FProperty>(Target->GetClass(), *PName);
		if (!Prop)
		{
			OutError = FString::Printf(TEXT("Property not found: %s"), *PName);
			return false;
		}

		void* PropData = Prop->ContainerPtrToValuePtr<void>(Target);
		if (!PropData)
		{
			OutError = FString::Printf(TEXT("Property data missing: %s"), *PName);
			return false;
		}

		Prop->ImportText_Direct(*ImportText, PropData, Target, PPF_None);
		return true;
	}
}
