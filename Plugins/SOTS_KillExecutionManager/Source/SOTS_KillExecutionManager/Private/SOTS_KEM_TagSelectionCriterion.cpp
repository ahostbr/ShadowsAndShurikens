#include "SOTS_KEM_TagSelectionCriterion.h"
#include "ContextualAnimTypes.h" // FContextualAnimSceneBindingContext

USOTS_KEM_TagSelectionCriterion::USOTS_KEM_TagSelectionCriterion(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool USOTS_KEM_TagSelectionCriterion::PassesTagTest(
    const FGameplayTagContainer& OwnedTags,
    const FGameplayTagContainer& Required,
    ESOTS_KEM_TagMatchType MatchType)
{
    if (Required.IsEmpty())
    {
        // No requirements → auto-pass
        return true;
    }

    switch (MatchType)
    {
    case ESOTS_KEM_TagMatchType::Any:
        return OwnedTags.HasAnyExact(Required);

    case ESOTS_KEM_TagMatchType::All:
        return OwnedTags.HasAllExact(Required);

    case ESOTS_KEM_TagMatchType::Exact:
        return OwnedTags == Required;

    default:
        return false;
    }
}

bool USOTS_KEM_TagSelectionCriterion::DoesQuerierPassCondition(
    const FContextualAnimSceneBindingContext& PrimaryCtx,
    const FContextualAnimSceneBindingContext& QuerierCtx) const
{
    // ContextualAnim merges external + actor tags inside this container
    const FGameplayTagContainer& PrimaryTags = PrimaryCtx.GetGameplayTags();
    const FGameplayTagContainer& QuerierTags = QuerierCtx.GetGameplayTags();

    // For "Context" we can just use the Querier tags again by default.
    // If you later push special KEM tags into the Querier's ExternalGameplayTags,
    // they'll appear here too.
    const FGameplayTagContainer& ContextTags = QuerierCtx.GetGameplayTags();

    // Primary gate (victim)
    if (!PassesTagTest(PrimaryTags, RequiredPrimaryTags, PrimaryMatchType))
    {
        return false;
    }

    // Querier gate (instigator)
    if (!PassesTagTest(QuerierTags, RequiredQuerierTags, QuerierMatchType))
    {
        return false;
    }

    // Optional context gate
    if (!RequiredContextTags.IsEmpty())
    {
        if (!PassesTagTest(ContextTags, RequiredContextTags, ContextMatchType))
        {
            return false;
        }
    }

    // Optional single tag check
    if (OptionalSingleRequiredTag.IsValid())
    {
        if (!PrimaryTags.HasTagExact(OptionalSingleRequiredTag) &&
            !QuerierTags.HasTagExact(OptionalSingleRequiredTag) &&
            !ContextTags.HasTagExact(OptionalSingleRequiredTag))
        {
            return false;
        }
    }

    return true;
}
