#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_BodyDragInteractionBridgeSubsystem.generated.h"

class USOTS_InteractionSubsystem;
struct FSOTS_InteractionActionRequest;

/**
 * Bridges canonical Interaction action requests into BodyDrag consumers.
 * Keeps Interaction decoupled while allowing drag verbs to be handled by this plugin.
 */
UCLASS()
class SOTS_BODYDRAG_API USOTS_BodyDragInteractionBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

private:
    bool TryBindInteractionSubsystem();
    void ScheduleRetryBind();
    void CancelRetryBind();
    void OnRetryBindTick();

    UFUNCTION()
    void HandleInteractionActionRequested(const FSOTS_InteractionActionRequest& Request);

    TWeakObjectPtr<USOTS_InteractionSubsystem> CachedInteractionSubsystem;
    bool bBoundToInteraction = false;
    bool bLoggedBindingFailure = false;
    FTimerHandle BindingRetryHandle;
};
