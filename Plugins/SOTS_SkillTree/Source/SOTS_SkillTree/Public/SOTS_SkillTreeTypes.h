#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_SkillTreeTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ESOTS_SkillNodeStatus : uint8
{
    Locked    UMETA(DisplayName="Locked"),
    Available UMETA(DisplayName="Available"),
    Unlocked  UMETA(DisplayName="Unlocked")
};

UENUM(BlueprintType)
enum class ESOTS_SkillNodeType : uint8
{
    Passive       UMETA(DisplayName="Passive"),
    ActiveAbility UMETA(DisplayName="Active Ability"),
    Upgrade       UMETA(DisplayName="Upgrade/Modifier"),
    Utility       UMETA(DisplayName="Utility"),
};

UENUM(BlueprintType)
enum class ESOTS_SkillUnlockRule : uint8
{
    AllParents     UMETA(DisplayName="All Parents Required"),
    AnyParent      UMETA(DisplayName="Any Parent Required"),
    NoParentNeeded UMETA(DisplayName="No Parents Required"),
};

// Data-only description of what is required to unlock a given node.
USTRUCT(BlueprintType)
struct FSOTS_SkillNodeRequirements
{
    GENERATED_BODY()

    // Generic global/player tags required to unlock this node. These are
    // evaluated via the global tag manager (e.g. difficulty, mission flags).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Requirements")
    FGameplayTagContainer RequiredTags;

    // Ability tags that should already be owned / granted before this node
    // can be unlocked. In the current SOTS project these are usually mirrored
    // as global player tags when abilities are granted.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Requirements")
    FGameplayTagContainer RequiredAbilityTags;

    // Mission-related tags that must be present (e.g. Mission.Castle.Completed).
    // These are also evaluated via the global tag manager and are authored
    // in MissionDirector or higher-level content.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Requirements")
    FGameplayTagContainer RequiredMissionTags;

    // Skill points required to unlock this node. If set to a value > 0,
    // this overrides the legacy Cost field on FSOTS_SkillNodeDefinition.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Requirements", meta=(ClampMin="0"))
    int32 RequiredPoints = 0;
};

// Data-only description of what happens when a node is unlocked.
USTRUCT(BlueprintType)
struct FSOTS_SkillNodeEffects
{
    GENERATED_BODY()

    // Tags granted when this node is unlocked (e.g., stat modifiers,
    // long-lived progression flags). These are pushed into the global
    // tag state by the skill tree subsystem.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Effects")
    FGameplayTagContainer GrantedTags;

    // Ability tags that represent abilities unlocked by this node. The
    // SkillTree subsystem itself only exposes these as data; higher-level
    // systems (or GAS bridges) can listen for node unlocks and grant
    // the actual abilities via UAC_SOTS_Abilitys.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Effects")
    FGameplayTagContainer GrantedAbilityTags;

    // Optional FX tag fired via the global FX manager when the node is
    // successfully unlocked. This is purely descriptive; listeners are
    // responsible for mapping it to actual Niagara/Sound assets.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Effects")
    FGameplayTag FXTag_OnUnlock;
};

USTRUCT(BlueprintType)
struct FSOTS_SkillNodeDefinition
{
    GENERATED_BODY()

    // Unique identifier for this node within the tree
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FName NodeId;

    // Display name for UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FText DisplayName;

    // Description for UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree", meta=(MultiLine="true"))
    FText Description;

    // Optional icon for UI representation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    TObjectPtr<UTexture2D> Icon = nullptr;

    // High-level node type (passive, active ability, upgrade, etc.).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    ESOTS_SkillNodeType NodeType = ESOTS_SkillNodeType::Passive;

    // 2D editor/UI position for layout tools. This is not used for
    // gameplay and can safely be ignored by runtime systems.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Layout")
    FVector2D EditorPosition = FVector2D::ZeroVector;

    // Parent/child wiring (by node id). If empty, falls back to PrerequisiteNodeIds.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    TArray<FName> ParentNodeIds;

    // Existing prerequisite list kept for backwards-compatibility.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    TArray<FName> PrerequisiteNodeIds;

    // Unlock rule controlling how parents are evaluated.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    ESOTS_SkillUnlockRule UnlockRule = ESOTS_SkillUnlockRule::AllParents;

    // Cost in skill points to unlock this node
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree", meta=(ClampMin="0"))
    int32 Cost = 1;

    // Tags that must be present on the player to unlock this node (difficulty, mission flags, etc).
    // Integration point: SOTS can query its global tag provider to validate these.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Tags")
    FGameplayTagContainer RequiredTags;

    // Primary gameplay tag granted when this node is unlocked (e.g. Skill.Stealth.Backstab).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Tags")
    FGameplayTag SkillTag;

    // Tags granted when this node is unlocked (e.g., abilities, stat modifiers, progression flags).
    // Integration point: SOTS can listen for node unlocks and apply these tags.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree|Tags")
    FGameplayTagContainer GrantedTags;

    // If true, this node may be refunded (respecced) via the subsystem/component APIs.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    bool bRefundable = true;

    // New, consolidated requirements and effects containers. Existing fields
    // (Cost, RequiredTags, GrantedTags) are kept for backward-compatibility;
    // when Requirements.RequiredPoints is > 0 it is treated as the primary
    // authored cost, and Effects.GrantedTags is appended to GrantedTags.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FSOTS_SkillNodeRequirements Requirements;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FSOTS_SkillNodeEffects Effects;
};

USTRUCT(BlueprintType)
struct FSOTS_SkillNodeState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FName NodeId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    ESOTS_SkillNodeStatus Status = ESOTS_SkillNodeStatus::Locked;
};

// Minimal runtime representation of an unlocked node.
USTRUCT(BlueprintType)
struct FSOTS_UnlockedSkillNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    FName NodeId;

    // Future: rank/level. For now, just 1 = unlocked.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    int32 Rank = 1;
};

USTRUCT(BlueprintType)
struct FSOTS_SkillTreeRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    FName TreeId;

    // Unlocked node ids (and optional rank data).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    TArray<FSOTS_UnlockedSkillNode> UnlockedNodes;

    // Optional point pool associated with this tree (for save/load friendliness).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    int32 AvailablePoints = 0;
};

// Lightweight view used by UI to query a tree's nodes along with their
// current unlock status and effective cost.
USTRUCT(BlueprintType)
struct FSOTS_SkillTreeNodeView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    FSOTS_SkillNodeDefinition Definition;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    ESOTS_SkillNodeStatus Status = ESOTS_SkillNodeStatus::Locked;

    // Effective cost after applying Requirements / legacy Cost fields.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|SkillTree")
    int32 EffectiveCost = 0;
};

// Profile-level snapshot that can be serialized by an external save system.
USTRUCT(BlueprintType)
struct FSOTS_SkillTreeProfileState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="SOTS|SkillTree")
    TArray<FSOTS_SkillTreeRuntimeState> Trees;
};

// Lightweight edge description for UI/layout tooling. Runtime logic primarily
// relies on per-node ParentNodeIds, but trees may also author explicit links.
USTRUCT(BlueprintType)
struct FSOTS_SkillNodeLink
{
    GENERATED_BODY()

    // Parent (source) node id.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FName ParentNodeId = NAME_None;

    // Child (destination) node id.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|SkillTree")
    FName ChildNodeId = NAME_None;
};
