#pragma once

#include "CoreMinimal.h"
#include "ContextualAnimSelectionCriterion.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_TagSelectionCriterion.generated.h"

UENUM(BlueprintType)
enum class ESOTS_KEM_TagMatchType : uint8
{
    Any   UMETA(DisplayName="Any (at least one)"),
    All   UMETA(DisplayName="All"),
    Exact UMETA(DisplayName="Exact (containers must match)"),
};

/**
 * Tag-based selection criterion for CAS variants used by KEM.
 * Evaluates gameplay tags on the Primary and Querier binding contexts.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta=(DisplayName="SOTS Gameplay Tag Criterion"))
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_TagSelectionCriterion
    : public UContextualAnimSelectionCriterion
{
    GENERATED_BODY()

public:
    USOTS_KEM_TagSelectionCriterion(const FObjectInitializer& ObjectInitializer);

    // Primary (usually victim) requirements

    /** Tags the Primary binding (victim) must satisfy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Primary")
    FGameplayTagContainer RequiredPrimaryTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Primary")
    ESOTS_KEM_TagMatchType PrimaryMatchType = ESOTS_KEM_TagMatchType::All;

    // Querier / Instigator requirements

    /** Tags the Querier binding (instigator / player) must satisfy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Querier")
    FGameplayTagContainer RequiredQuerierTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Querier")
    ESOTS_KEM_TagMatchType QuerierMatchType = ESOTS_KEM_TagMatchType::All;

    // Optional context tags (extra KEM / state tags)

    /** Tags that must be present in the binding's gameplay tags (eg. KEM execution context tags). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
    FGameplayTagContainer RequiredContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
    ESOTS_KEM_TagMatchType ContextMatchType = ESOTS_KEM_TagMatchType::All;

    /** Optional single tag that must appear on at least one of Primary / Querier / Context. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
    FGameplayTag OptionalSingleRequiredTag;

    // UContextualAnimSelectionCriterion interface
    virtual bool DoesQuerierPassCondition(
        const FContextualAnimSceneBindingContext& Primary,
        const FContextualAnimSceneBindingContext& Querier) const override;

protected:
    static bool PassesTagTest(
        const FGameplayTagContainer& OwnedTags,
        const FGameplayTagContainer& Required,
        ESOTS_KEM_TagMatchType MatchType);
};
