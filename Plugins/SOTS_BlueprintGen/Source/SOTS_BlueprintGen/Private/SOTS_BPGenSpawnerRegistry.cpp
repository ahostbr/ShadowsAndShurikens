#include "SOTS_BPGenSpawnerRegistry.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "SOTS_BlueprintGen.h"

#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	static TMap<FString, FSOTS_BPGenResolvedSpawner> GSOTS_BPGenSpawnerCache;

	static FString BuildSpawnerKey(UBlueprintNodeSpawner* Spawner)
	{
		if (!Spawner)
		{
			return FString();
		}

		if (UBlueprintFunctionNodeSpawner* FuncSpawner = Cast<UBlueprintFunctionNodeSpawner>(Spawner))
		{
			if (UFunction* Function = FuncSpawner->GetFunction())
			{
				return Function->GetPathName();
			}
		}

		if (UBlueprintVariableNodeSpawner* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(Spawner))
		{
			FString OwnerPath;
			if (UStruct* OwnerStruct = VarSpawner->GetVarOuter())
			{
				OwnerPath = OwnerStruct->GetPathName();
			}

			if (!OwnerPath.IsEmpty())
			{
				return FString::Printf(TEXT("%s:%s"), *OwnerPath, *VarSpawner->GetVarName().ToString());
			}
		}

		if (UClass* NodeClass = Spawner->NodeClass)
		{
			if (NodeClass->GetName().Equals(TEXT("K2Node_DynamicCast")))
			{
				return FString::Printf(TEXT("K2Node_DynamicCast|%s"), *NodeClass->GetPathName());
			}

			return NodeClass->GetPathName();
		}

		return FString();
	}

	static bool ResolveFromActionDatabase(const FString& SpawnerKey, FSOTS_BPGenResolvedSpawner& OutResolved)
	{
		FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
		const FBlueprintActionDatabase::FActionRegistry& Registry = ActionDB.GetAllActions();

		for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Entry : Registry)
		{
			for (UBlueprintNodeSpawner* Spawner : Entry.Value)
			{
				if (!Spawner)
				{
					continue;
				}

				const FString CandidateKey = BuildSpawnerKey(Spawner);
				if (CandidateKey.IsEmpty())
				{
					continue;
				}

				if (CandidateKey.Equals(SpawnerKey, ESearchCase::CaseSensitive))
				{
					OutResolved.SpawnerKey = SpawnerKey;
					OutResolved.Spawner = Spawner;
					OutResolved.DebugName = Spawner->NodeClass ? Spawner->NodeClass->GetName() : SpawnerKey;
					OutResolved.DebugCategory = TEXT("ActionDatabase");
					return true;
				}
			}
		}

		return false;
	}
}

bool FSOTS_BPGenSpawnerRegistry::ResolveForContext(UBlueprint* Blueprint, UEdGraph* Graph, const FString& SpawnerKey, FSOTS_BPGenResolvedSpawner& OutResolved, FString& OutError)
{
	OutResolved = FSOTS_BPGenResolvedSpawner();

	if (SpawnerKey.IsEmpty())
	{
		OutError = TEXT("SpawnerKey is empty.");
		return false;
	}

	if (const FSOTS_BPGenResolvedSpawner* Cached = GSOTS_BPGenSpawnerCache.Find(SpawnerKey))
	{
		if (Cached->Spawner.IsValid())
		{
			OutResolved = *Cached;
			return true;
		}
		GSOTS_BPGenSpawnerCache.Remove(SpawnerKey);
	}

	// Minimal SPINE_B support: treat key as function path.
	UFunction* TargetFunction = FindObject<UFunction>(nullptr, *SpawnerKey);
	if (!TargetFunction)
	{
		TargetFunction = LoadObject<UFunction>(nullptr, *SpawnerKey);
	}

	if (TargetFunction)
	{
		if (UBlueprintFunctionNodeSpawner* FunctionSpawner = UBlueprintFunctionNodeSpawner::Create(TargetFunction))
		{
			FSOTS_BPGenResolvedSpawner Entry;
			Entry.SpawnerKey = SpawnerKey;
			Entry.Spawner = FunctionSpawner;
			Entry.DebugName = TargetFunction->GetName();
			Entry.DebugCategory = TargetFunction->GetOuter() ? TargetFunction->GetOuter()->GetName() : TEXT("Function");

			GSOTS_BPGenSpawnerCache.Add(SpawnerKey, Entry);
			OutResolved = Entry;
			return true;
		}

		OutError = FString::Printf(TEXT("Failed to create function spawner for '%s'."), *SpawnerKey);
		return false;
	}

	if (ResolveFromActionDatabase(SpawnerKey, OutResolved))
	{
		GSOTS_BPGenSpawnerCache.Add(SpawnerKey, OutResolved);
		return true;
	}

	OutError = FString::Printf(TEXT("SpawnerKey '%s' could not be resolved by the spawner registry."), *SpawnerKey);
	return false;
}

void FSOTS_BPGenSpawnerRegistry::Clear()
{
	GSOTS_BPGenSpawnerCache.Empty();
}
