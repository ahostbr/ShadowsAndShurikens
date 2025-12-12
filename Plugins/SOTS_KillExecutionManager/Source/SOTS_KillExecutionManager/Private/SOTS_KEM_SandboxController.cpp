#include "SOTS_KEM_SandboxController.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "SOTS_KEM_ManagerSubsystem.h"

namespace
{
    int32 WrapIndex(int32 Value, int32 Size)
    {
        if (Size <= 0)
        {
            return 0;
        }

        Value %= Size;
        if (Value < 0)
        {
            Value += Size;
        }
        return Value;
    }
}

ASOTS_KEMSandboxController::ASOTS_KEMSandboxController()
{
    PrimaryActorTick.bCanEverTick = false;
    PlayerSpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
}

void ASOTS_KEMSandboxController::BeginPlay()
{
    Super::BeginPlay();
    BindTelemetryDelegate();
    ResetScenario();
}

void ASOTS_KEMSandboxController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindTelemetryDelegate();
    if (SpawnedPlayerPawn)
    {
        if (AController* Controller = SpawnedPlayerPawn->GetController())
        {
            Controller->UnPossess();
        }
        SpawnedPlayerPawn->Destroy();
        SpawnedPlayerPawn = nullptr;
    }
    if (SpawnedTargetActor)
    {
        SpawnedTargetActor->Destroy();
        SpawnedTargetActor = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void ASOTS_KEMSandboxController::CycleExecution(int32 Delta)
{
    if (ExecutionDefinitions.Num() == 0)
    {
        return;
    }

    CurrentExecutionIndex = WrapIndex(CurrentExecutionIndex + Delta, ExecutionDefinitions.Num());
    ResetScenario();
}

void ASOTS_KEMSandboxController::CyclePosition(int32 Delta)
{
    if (PositionPresets.Num() == 0)
    {
        return;
    }

    CurrentPositionIndex = WrapIndex(CurrentPositionIndex + Delta, PositionPresets.Num());
    ResetScenario();
}

void ASOTS_KEMSandboxController::ResetScenario()
{
    if (SpawnedPlayerPawn)
    {
        if (AController* Controller = SpawnedPlayerPawn->GetController())
        {
            Controller->UnPossess();
        }
        SpawnedPlayerPawn->Destroy();
        SpawnedPlayerPawn = nullptr;
    }

    if (SpawnedTargetActor)
    {
        SpawnedTargetActor->Destroy();
        SpawnedTargetActor = nullptr;
    }

    SpawnPlayerPawn();
    SpawnTargetActor();
}

void ASOTS_KEMSandboxController::TriggerKEMExecution()
{
    if (!SpawnedPlayerPawn || !SpawnedTargetActor)
    {
        return;
    }

    USOTS_KEMManagerSubsystem* KEM = GetKEMSubsystem();
    if (!KEM)
    {
        return;
    }

    if (ExecutionDefinitions.Num() == 0)
    {
        return;
    }

    USOTS_KEM_ExecutionDefinition* Definition = GetCurrentExecutionDefinition();
    if (!Definition)
    {
        return;
    }

    FGameplayTagContainer ContextTags = SandboxContextTags;
    const FSOTS_KEMSandboxPosition* CurrentPreset = GetCurrentPositionPreset();
    if (CurrentPreset && CurrentPreset->PositionTag.IsValid())
    {
        ContextTags.AddTag(CurrentPreset->PositionTag);
    }

    const FString SourceLabel = CurrentPreset && !CurrentPreset->Name.IsNone()
        ? CurrentPreset->Name.ToString()
        : TEXT("KEM Sandbox");

    KEM->RequestExecution(this,
                          SpawnedPlayerPawn,
                          SpawnedTargetActor,
                          ContextTags,
                          Definition,
                          SourceLabel);
}

const FSOTS_KEMExecutionTelemetry& ASOTS_KEMSandboxController::GetLastExecutionTelemetry() const
{
    return LastTelemetry;
}

void ASOTS_KEMSandboxController::SpawnPlayerPawn()
{
    if (!PlayerPawnClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
    {
        return;
    }

    SpawnedPlayerPawn = World->SpawnActor<APawn>(PlayerPawnClass, PlayerSpawnTransform);
    if (SpawnedPlayerPawn)
    {
        PC->Possess(SpawnedPlayerPawn);
    }
}

void ASOTS_KEMSandboxController::SpawnTargetActor()
{
    if (!TargetClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FTransform TargetTransform = GetTargetTransform();
    SpawnedTargetActor = World->SpawnActor<AActor>(TargetClass, TargetTransform);
}

FTransform ASOTS_KEMSandboxController::GetTargetTransform() const
{
    const FTransform BaseTransform = PlayerSpawnTransform;
    const FSOTS_KEMSandboxPosition* Preset = GetCurrentPositionPreset();
    if (!Preset)
    {
        return BaseTransform;
    }

    const FQuat Rotation = BaseTransform.GetRotation() * Preset->RelativeRotation.Quaternion();
    const FVector Location = BaseTransform.GetLocation() + BaseTransform.GetRotation().RotateVector(Preset->RelativeLocation);
    return FTransform(Rotation, Location);
}

USOTS_KEM_ExecutionDefinition* ASOTS_KEMSandboxController::GetCurrentExecutionDefinition() const
{
    if (ExecutionDefinitions.Num() == 0)
    {
        return nullptr;
    }

    const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDefinition = ExecutionDefinitions[CurrentExecutionIndex];
    USOTS_KEM_ExecutionDefinition* Definition = SoftDefinition.Get();
    if (!Definition && SoftDefinition.ToSoftObjectPath().IsValid())
    {
        Definition = SoftDefinition.LoadSynchronous();
    }

    return Definition;
}

const FSOTS_KEMSandboxPosition* ASOTS_KEMSandboxController::GetCurrentPositionPreset() const
{
    if (PositionPresets.Num() == 0)
    {
        return nullptr;
    }

    int32 Index = WrapIndex(CurrentPositionIndex, PositionPresets.Num());
    return &PositionPresets[Index];
}

void ASOTS_KEMSandboxController::HandleTelemetry(const FSOTS_KEMExecutionTelemetry& Telemetry)
{
    LastTelemetry = Telemetry;
}

void ASOTS_KEMSandboxController::BindTelemetryDelegate()
{
    if (USOTS_KEMManagerSubsystem* KEM = GetKEMSubsystem())
    {
        FDelegateHandle Handle = KEM->GetTelemetryDelegate().AddUObject(this, &ASOTS_KEMSandboxController::HandleTelemetry);
        TelemetryDelegateHandle = Handle;
    }
}

void ASOTS_KEMSandboxController::UnbindTelemetryDelegate()
{
    if (USOTS_KEMManagerSubsystem* KEM = GetKEMSubsystem())
    {
        KEM->GetTelemetryDelegate().Remove(TelemetryDelegateHandle);
    }
}

USOTS_KEMManagerSubsystem* ASOTS_KEMSandboxController::GetKEMSubsystem() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        return GI->GetSubsystem<USOTS_KEMManagerSubsystem>();
    }

    return nullptr;
}
