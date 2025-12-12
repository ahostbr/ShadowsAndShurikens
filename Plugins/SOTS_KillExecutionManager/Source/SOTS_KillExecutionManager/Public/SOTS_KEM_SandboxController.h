#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_KEM_SandboxController.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_KEMSandboxPosition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox")
    FName Name = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox")
    FGameplayTag PositionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox")
    FVector RelativeLocation = FVector(300.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox")
    FRotator RelativeRotation = FRotator::ZeroRotator;
};

UCLASS(Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API ASOTS_KEMSandboxController : public AActor
{
    GENERATED_BODY()

public:
    ASOTS_KEMSandboxController();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    TArray<TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>> ExecutionDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    TArray<FSOTS_KEMSandboxPosition> PositionPresets;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    TSubclassOf<APawn> PlayerPawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    TSubclassOf<AActor> TargetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    FTransform PlayerSpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sandbox")
    FGameplayTagContainer SandboxContextTags;

    UPROPERTY(BlueprintReadOnly, Category="Sandbox")
    int32 CurrentExecutionIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="Sandbox")
    int32 CurrentPositionIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="Sandbox")
    FSOTS_KEMExecutionTelemetry LastTelemetry;

    UFUNCTION(BlueprintCallable, Category="Sandbox")
    void CycleExecution(int32 Delta);

    UFUNCTION(BlueprintCallable, Category="Sandbox")
    void CyclePosition(int32 Delta);

    UFUNCTION(BlueprintCallable, Category="Sandbox")
    void ResetScenario();

    UFUNCTION(BlueprintCallable, Category="Sandbox")
    void TriggerKEMExecution();

    UFUNCTION(BlueprintPure, Category="Sandbox")
    const FSOTS_KEMExecutionTelemetry& GetLastExecutionTelemetry() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SpawnPlayerPawn();
    void SpawnTargetActor();
    FTransform GetTargetTransform() const;
    USOTS_KEM_ExecutionDefinition* GetCurrentExecutionDefinition() const;
    const FSOTS_KEMSandboxPosition* GetCurrentPositionPreset() const;
    void HandleTelemetry(const FSOTS_KEMExecutionTelemetry& Telemetry);
    void BindTelemetryDelegate();
    void UnbindTelemetryDelegate();

    USOTS_KEMManagerSubsystem* GetKEMSubsystem() const;

    UPROPERTY(Transient)
    APawn* SpawnedPlayerPawn = nullptr;

    UPROPERTY(Transient)
    AActor* SpawnedTargetActor = nullptr;

    FDelegateHandle TelemetryDelegateHandle;
};
