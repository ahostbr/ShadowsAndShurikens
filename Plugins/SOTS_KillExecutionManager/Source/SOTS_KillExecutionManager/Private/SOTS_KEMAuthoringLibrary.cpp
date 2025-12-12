#include "SOTS_KEMAuthoringLibrary.h"

#include "Containers/Map.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "UObject/UObjectIterator.h"

namespace
{
    struct FCoverageKey
    {
        FGameplayTag FamilyTag;
        FGameplayTag PositionTag;
        ESOTS_KEMTargetKind TargetKind = ESOTS_KEMTargetKind::Generic;

        bool operator==(const FCoverageKey& Other) const
        {
            return FamilyTag == Other.FamilyTag && PositionTag == Other.PositionTag &&
                   TargetKind == Other.TargetKind;
        }
    };

    inline uint32 GetTypeHash(const FCoverageKey& Key)
    {
        uint32 Hash = HashCombine(GetTypeHash(Key.FamilyTag), GetTypeHash(Key.PositionTag));
        return HashCombine(Hash, static_cast<uint32>(Key.TargetKind));
    }
}

namespace
{
    template<typename Pred>
    void FilterDefinitions(
        const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
        TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs,
        Pred Predicate
    )
    {
        OutDefs.Reset();
        for (USOTS_KEM_ExecutionDefinition* Definition : InDefs)
        {
            if (!Definition)
            {
                continue;
            }

            if (Predicate(Definition))
            {
                OutDefs.Add(Definition);
            }
        }
    }

    bool MatchesFamilyTag(const USOTS_KEM_ExecutionDefinition* Definition, const FGameplayTag& FamilyTag)
    {
        if (!FamilyTag.IsValid())
        {
            return true;
        }
        return Definition->ExecutionFamilyTag.MatchesTag(FamilyTag);
    }

    bool MatchesPositionTag(const USOTS_KEM_ExecutionDefinition* Definition, const FGameplayTag& PositionTag)
    {
        if (!PositionTag.IsValid())
        {
            return true;
        }

        if (Definition->PositionTag.MatchesTag(PositionTag))
        {
            return true;
        }

        for (const FGameplayTag& Additional : Definition->AdditionalPositionTags)
        {
            if (Additional.MatchesTag(PositionTag))
            {
                return true;
            }
        }

        return false;
    }

    bool MatchesTargetKind(const USOTS_KEM_ExecutionDefinition* Definition, ESOTS_KEMTargetKind TargetKind)
    {
        return Definition->Authoring.TargetKind == TargetKind;
    }
}

void USOTS_KEMAuthoringLibrary::GetAllExecutionDefinitions(TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs)
{
    OutDefs.Reset();

    for (TObjectIterator<USOTS_KEM_ExecutionDefinition> It; It; ++It)
    {
        if (USOTS_KEM_ExecutionDefinition* Definition = *It)
        {
            if (Definition->HasAllFlags(RF_ClassDefaultObject))
            {
                continue;
            }
            OutDefs.Add(Definition);
        }
    }
}

void USOTS_KEMAuthoringLibrary::FilterExecutionDefinitionsByFamily(
    const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
    FGameplayTag FamilyTag,
    TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs)
{
    FilterDefinitions(InDefs, OutDefs, [&](const USOTS_KEM_ExecutionDefinition* Definition) {
        return MatchesFamilyTag(Definition, FamilyTag);
    });
}

void USOTS_KEMAuthoringLibrary::FilterExecutionDefinitionsByPosition(
    const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
    FGameplayTag PositionTag,
    TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs)
{
    FilterDefinitions(InDefs, OutDefs, [&](const USOTS_KEM_ExecutionDefinition* Definition) {
        return MatchesPositionTag(Definition, PositionTag);
    });
}

void USOTS_KEMAuthoringLibrary::FilterExecutionDefinitionsByTargetKind(
    const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
    ESOTS_KEMTargetKind TargetKind,
    TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs)
{
    FilterDefinitions(InDefs, OutDefs, [&](const USOTS_KEM_ExecutionDefinition* Definition) {
        return MatchesTargetKind(Definition, TargetKind);
    });
}

void USOTS_KEMAuthoringLibrary::BuildCoverageMatrix(
    const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
    TArray<FSOTS_KEMCoverageCell>& OutCells)
{
    TMap<FCoverageKey, FSOTS_KEMCoverageCell> Coverage;
    Coverage.Reserve(InDefs.Num());

    for (USOTS_KEM_ExecutionDefinition* Definition : InDefs)
    {
        if (!Definition)
        {
            continue;
        }

        const FCoverageKey Key{
            Definition->ExecutionFamilyTag,
            Definition->PositionTag,
            Definition->Authoring.TargetKind
        };

        FSOTS_KEMCoverageCell& Cell = Coverage.FindOrAdd(Key);
        Cell.ExecutionFamilyTag = Definition->ExecutionFamilyTag;
        Cell.PositionTag = Definition->PositionTag;
        Cell.TargetKind = Definition->Authoring.TargetKind;
        Cell.DefinitionCount++;
        Cell.bHasAnyBossOnly = Cell.bHasAnyBossOnly || Definition->Authoring.bIsBossOnly;
        Cell.bHasAnyDragonOnly = Cell.bHasAnyDragonOnly || Definition->Authoring.bIsDragonOnly;
    }

    OutCells.Reset();
    OutCells.Reserve(Coverage.Num());
    for (auto& Pair : Coverage)
    {
        OutCells.Add(Pair.Value);
    }
}

void USOTS_KEMAuthoringLibrary::LogCoverageMatrix(const TArray<FSOTS_KEMCoverageCell>& Cells)
{
    const UEnum* TargetKindEnum = StaticEnum<ESOTS_KEMTargetKind>();

    for (const FSOTS_KEMCoverageCell& Cell : Cells)
    {
        const FString Family = Cell.ExecutionFamilyTag.IsValid() ? Cell.ExecutionFamilyTag.ToString() : TEXT("None");
        const FString Position = Cell.PositionTag.IsValid() ? Cell.PositionTag.ToString() : TEXT("None");
        const FString TargetKindName = TargetKindEnum
            ? TargetKindEnum->GetNameStringByValue(static_cast<int64>(Cell.TargetKind))
            : TEXT("Unknown");

        UE_LOG(LogSOTS_KEM, Log, TEXT("[KEM_COV] Family=%s Position=%s Target=%s Count=%d Boss=%d Dragon=%d"),
            *Family,
            *Position,
            *TargetKindName,
            Cell.DefinitionCount,
            Cell.bHasAnyBossOnly ? 1 : 0,
            Cell.bHasAnyDragonOnly ? 1 : 0);
    }
}
