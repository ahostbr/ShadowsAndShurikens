#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_SkillTreeTypes.h"
#include "SOTS_SkillTreeDefinition.generated.h"

/**
 * Data asset describing a single skill tree and all of its nodes.
 * This is content-only and safe to author entirely in the editor.
 */
UCLASS(BlueprintType)
class SOTS_SKILLTREE_API USOTS_SkillTreeDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Optional id so multiple trees can be referenced in data
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SkillTree")
    FName TreeId;

    // All nodes that belong to this tree
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SkillTree")
    TArray<FSOTS_SkillNodeDefinition> Nodes;

    // Optional explicit link list for editor tooling. Runtime unlock logic
    // currently uses the per-node ParentNodeIds/PrerequisiteNodeIds fields,
    // but UI may prefer an explicit edge list when drawing the graph.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SkillTree")
    TArray<FSOTS_SkillNodeLink> Links;

    /**
     * Looks up a node definition by id.
     * Returns true if found and copies the data into OutDefinition.
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    bool FindNodeDefinition(FName NodeId, FSOTS_SkillNodeDefinition& OutDefinition) const;
};
