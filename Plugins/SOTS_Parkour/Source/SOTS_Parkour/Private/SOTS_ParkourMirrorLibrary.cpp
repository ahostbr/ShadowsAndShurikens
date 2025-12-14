#include "SOTS_ParkourMirrorLibrary.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	static TMap<FName, FName> GMirrorMap;
	static bool GMirrorMapLoaded = false;
	static FCriticalSection GMirrorMapCS;

	bool LoadMirrorMapInternal()
	{
		if (GMirrorMapLoaded)
		{
			return GMirrorMap.Num() > 0;
		}

		FScopeLock Lock(&GMirrorMapCS);
		if (GMirrorMapLoaded)
		{
			return GMirrorMap.Num() > 0;
		}

		GMirrorMap.Reset();

		// Prefer a copy under Content/SOTS/Data if it exists; otherwise fall back to the exported JSON in BlueprintExports.
		const FString CandidatePaths[] = {
			FPaths::ProjectContentDir() / TEXT("SOTS/Data/NewMirrorDataTable.json"),
			FPaths::ProjectDir() / TEXT("BlueprintExports/NewMirrorDataTable.json"),
		};

		for (const FString& Path : CandidatePaths)
		{
			FString JsonText;
			if (!FPaths::FileExists(Path) || !FFileHelper::LoadFileToString(JsonText, *Path))
			{
				continue;
			}

			TSharedPtr<FJsonValue> RootValue;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
			if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid() || RootValue->Type != EJson::Array)
			{
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* ArrayPtr;
			if (!RootValue->TryGetArray(ArrayPtr))
			{
				continue;
			}

			for (const TSharedPtr<FJsonValue>& Value : *ArrayPtr)
			{
				if (!Value.IsValid() || Value->Type != EJson::Object)
				{
					continue;
				}

				const TSharedPtr<FJsonObject> EntryObj = Value->AsObject();
				FString NameStr;
				FString MirrorStr;
				if (!EntryObj->TryGetStringField(TEXT("Name"), NameStr) || !EntryObj->TryGetStringField(TEXT("MirroredName"), MirrorStr))
				{
					continue;
				}

				const FName Name(*NameStr);
				const FName Mirror(*MirrorStr);

				GMirrorMap.Add(Name, Mirror);
				// Ensure reverse lookup exists even if the JSON is one-way.
				GMirrorMap.FindOrAdd(Mirror) = Name;
			}

			GMirrorMapLoaded = true;
			break;
		}

		GMirrorMapLoaded = true;

		if (GMirrorMap.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("SOTS_ParkourMirrorLibrary: Failed to load NewMirrorDataTable.json from Content/SOTS/Data or BlueprintExports; falling back to heuristic _l/_r swaps."));
		}
		return GMirrorMap.Num() > 0;
	}

	FName HeuristicSwap(const FString& BoneNameStr)
	{
		if (BoneNameStr.EndsWith(TEXT("_l")))
		{
			return FName(*(BoneNameStr.LeftChop(2) + TEXT("_r")));
		}
		if (BoneNameStr.EndsWith(TEXT("_r")))
		{
			return FName(*(BoneNameStr.LeftChop(2) + TEXT("_l")));
		}
		return NAME_None;
	}
}

bool USOTS_ParkourMirrorLibrary::LoadMirrorMap()
{
	return LoadMirrorMapInternal();
}

const FName* USOTS_ParkourMirrorLibrary::FindMirror(FName BoneName)
{
	LoadMirrorMapInternal();
	return GMirrorMap.Find(BoneName);
}

FName USOTS_ParkourMirrorLibrary::GetMirroredBoneName(FName BoneName)
{
	if (BoneName.IsNone())
	{
		return NAME_None;
	}

	if (const FName* FromMap = FindMirror(BoneName))
	{
		return *FromMap;
	}

	const FName Heuristic = HeuristicSwap(BoneName.ToString());
	return Heuristic.IsNone() ? BoneName : Heuristic;
}

bool USOTS_ParkourMirrorLibrary::GetMirrorPair(FName BoneName, FName& OutPrimary, FName& OutMirror)
{
	OutPrimary = BoneName;
	OutMirror = NAME_None;

	if (BoneName.IsNone())
	{
		return false;
	}

	if (const FName* FromMap = FindMirror(BoneName))
	{
		OutMirror = *FromMap;
		return true;
	}

	const FName Heuristic = HeuristicSwap(BoneName.ToString());
	if (!Heuristic.IsNone())
	{
		OutMirror = Heuristic;
		return true;
	}

	return false;
}
