#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_KillExecutionManagerKEMAnchorDebugWidget.generated.h"

class AActor;

UCLASS(Abstract, Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KillExecutionManagerKEMAnchorDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    USOTS_KillExecutionManagerKEMAnchorDebugWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void SetCenterActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void SetSearchRadius(float Radius);

    UFUNCTION(BlueprintPure, Category="SOTS|KEM|Debug")
    const TArray<FSOTS_KEMAnchorDebugInfo>& GetAnchorInfos() const { return AnchorInfos; }

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    TArray<FSOTS_KEMAnchorDebugInfo> AnchorInfos;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Debug", meta=(ClampMin="0.0"))
    float SearchRadius = 600.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    TWeakObjectPtr<AActor> CenterActor;
};
