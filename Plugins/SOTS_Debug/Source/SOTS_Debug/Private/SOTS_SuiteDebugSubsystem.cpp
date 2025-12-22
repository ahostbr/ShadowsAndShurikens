// SPDX-License-Identifier: MIT
#include "SOTS_SuiteDebugSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "SOTS_TagAccessHelpers.h"
#include "SOTS_MissionDirectorSubsystem.h"
#include "SOTS_MMSSSubsystem.h"
#include "InvSPInventoryComponent.h"
#include "SOTS_StatsComponent.h"
#include "Components/ActorComponent.h"
#include "SOTS_TagLibrary.h"
#include "Components/SOTS_AbilityComponent.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_KillExecutionManagerKEMAnchorDebugWidget.h"
#include "Engine/Engine.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// Console toggle for all SOTS debug widgets (dev builds only, default off)
static TAutoConsoleVariable<int32> CVarSOTSDebugWidgets(
    TEXT("sots.DebugWidgets"),
    0,
    TEXT("Enable SOTS debug widgets (0=off, 1=on)."),
    ECVF_Default);

static void HandleSOTSDebugWidgetsChanged()
{
    if (CVarSOTSDebugWidgets.GetValueOnAnyThread() != 0)
    {
        return;
    }

    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (UWorld* World = Context.World())
            {
                if (UGameInstance* GI = World->GetGameInstance())
                {
                    if (USOTS_SuiteDebugSubsystem* Subsys = GI->GetSubsystem<USOTS_SuiteDebugSubsystem>())
                    {
                        Subsys->HideKEMAnchorOverlay();
                    }
                }
            }
        }
    }
}

static FAutoConsoleVariableSink SOTSDebugWidgetsSink(FConsoleCommandDelegate::CreateStatic(&HandleSOTSDebugWidgetsChanged));

namespace
{
    inline bool AreDebugWidgetsEnabled()
    {
        return CVarSOTSDebugWidgets.GetValueOnAnyThread() != 0;
    }
}
#endif

#include "UObject/UnrealType.h"

USOTS_SuiteDebugSubsystem* USOTS_SuiteDebugSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<USOTS_SuiteDebugSubsystem>();
}

AActor* USOTS_SuiteDebugSubsystem::GetPlayerPawn() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

FString USOTS_SuiteDebugSubsystem::GetGlobalStealthSummary() const
{
    USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(GetWorld());
    if (!GSM)
    {
        return TEXT("GSM: missing");
    }

    const float Score = GSM->GetCurrentStealthScore();
    const FString LevelName = StaticEnum<ESOTSStealthLevel>()->GetNameStringByValue((int64)GSM->GetCurrentStealthLevel());
    const bool bDetected = GSM->IsPlayerDetected();
    return FString::Printf(TEXT("GSM: Level=%s Score=%.2f Detected=%s"), *LevelName, Score, bDetected ? TEXT("Yes") : TEXT("No"));
}

FString USOTS_SuiteDebugSubsystem::GetMissionDirectorSummary() const
{
    USOTS_MissionDirectorSubsystem* MD = USOTS_MissionDirectorSubsystem::Get(GetWorld());
    if (!MD)
    {
        return TEXT("MissionDirector: missing");
    }

    const bool bActive = MD->IsMissionActive();
    const FName CurrentId = MD->CurrentMissionId;
    const float Score = MD->GetCurrentScore();
    const int32 Primary = MD->PrimaryObjectivesCompleted;
    const int32 Optional = MD->OptionalObjectivesCompleted;
    return FString::Printf(TEXT("Mission: Active=%s Id=%s Score=%.1f Primary=%d Optional=%d"), bActive ? TEXT("Yes") : TEXT("No"), *CurrentId.ToString(), Score, Primary, Optional);
}

FString USOTS_SuiteDebugSubsystem::GetMusicSubsystemSummary() const
{
    USOTS_MMSSSubsystem* MMSS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTS_MMSSSubsystem>() : nullptr;
    if (!MMSS)
    {
        return TEXT("MMSS: missing");
    }

    const FName MissionId = MMSS->GetCurrentMissionId();
    const FGameplayTag Track = MMSS->GetCurrentTrackId();
    const float Time = MMSS->GetCurrentPlaybackTime();
    return FString::Printf(TEXT("MMSS: Mission=%s Track=%s Time=%.2f"), *MissionId.ToString(), *Track.ToString(), Time);
}

FString USOTS_SuiteDebugSubsystem::GetTagManagerSummary() const
{
    USOTS_GameplayTagManagerSubsystem* TM = SOTS_GetTagSubsystem(GetWorld());
    if (!TM)
    {
        return TEXT("TagManager: missing");
    }

    // Do a modest probe for a commonly used sample tag; if it exists we show 'resolved'.
    const FGameplayTag ProbeTag = TM->GetTagByName(FName(TEXT("Player.States.Aiming")));
    const FString ProbeStr = ProbeTag.IsValid() ? TEXT("Found(Player.States.Aiming)") : TEXT("Probe not found");
    return FString::Printf(TEXT("TagManager: present; %s"), *ProbeStr);
}

FString USOTS_SuiteDebugSubsystem::GetInventorySummary() const
{
    AActor* Pawn = GetPlayerPawn();
    if (!Pawn)
    {
        return TEXT("Inventory: no player pawn");
    }

    UInvSP_InventoryComponent* InvComp = Pawn->FindComponentByClass<UInvSP_InventoryComponent>();
    if (!InvComp)
    {
        return TEXT("Inventory: missing component");
    }

    const int32 Carried = InvComp->GetCarriedItems().Num();
    const int32 Stash = InvComp->GetStashItems().Num();
    const int32 Quick = InvComp->GetQuickSlots().Num();
    return FString::Printf(TEXT("Inventory: Carried=%d Stash=%d QuickSlots=%d"), Carried, Stash, Quick);
}

FString USOTS_SuiteDebugSubsystem::GetStatsSummary(int32 TopN) const
{
    const TMap<FGameplayTag, float>* StatsMap = nullptr;
    if (AActor* Pawn = GetPlayerPawn())
    {
        if (USOTS_StatsComponent* Stats = Pawn->FindComponentByClass<USOTS_StatsComponent>())
        {
            StatsMap = &Stats->GetAllStats();
        }
    }

    static const TMap<FGameplayTag, float> EmptyStats;
    const TMap<FGameplayTag, float>& Map = StatsMap ? *StatsMap : EmptyStats;
    if (Map.Num() == 0)
    {
        return TEXT("Stats: none");
    }

    TArray<TPair<FGameplayTag, float>> Pairs;
    Pairs.Reserve(Map.Num());
    for (const auto& Pair : Map)
    {
        Pairs.Add(Pair);
    }
    Pairs.Sort([](const TPair<FGameplayTag, float>& A, const TPair<FGameplayTag, float>& B){ return A.Value > B.Value; });

    FString Result;
    const int32 Count = FMath::Min(TopN, Pairs.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        if (i) Result += TEXT(", ");
        Result += FString::Printf(TEXT("%s=%.2f"), *Pairs[i].Key.ToString(), Pairs[i].Value);
    }
    return FString::Printf(TEXT("Stats top %d: %s"), Count, *Result);
}

FString USOTS_SuiteDebugSubsystem::GetAbilitiesSummary() const
{
    AActor* Pawn = GetPlayerPawn();
    if (!Pawn)
    {
        return TEXT("Abilities: no player pawn");
    }

    UActorComponent* Comp = Pawn->FindComponentByClass<UAC_SOTS_Abilitys>();
    if (!Comp)
    {
        return TEXT("Abilities: component missing");
    }

    UAC_SOTS_Abilitys* AbComp = Cast<UAC_SOTS_Abilitys>(Comp);
    if (!AbComp)
    {
        return TEXT("Abilities: cannot cast");
    }

    TArray<FGameplayTag> Known;
    AbComp->GetKnownAbilities(Known);
    return FString::Printf(TEXT("Abilities known=%d"), Known.Num());
}

FString USOTS_SuiteDebugSubsystem::GetFXSummary() const
{
    USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get();
    if (!FX)
    {
        return TEXT("FX: missing");
    }

    FString Settings = FX->GetFXSettingsSummary();
    int32 ActiveNiagara = 0, ActiveAudio = 0;
    FX->GetActiveFXCountsInternal(ActiveNiagara, ActiveAudio);
    return FString::Printf(TEXT("FX: %s Niag=%d Audio=%d"), *Settings, ActiveNiagara, ActiveAudio);
}

void USOTS_SuiteDebugSubsystem::DumpSuiteStateToLog() const
{
    const FString GSM = GetGlobalStealthSummary();
    const FString Mission = GetMissionDirectorSummary();
    const FString MMSS = GetMusicSubsystemSummary();
    const FString Tag = GetTagManagerSummary();
    const FString Inv = GetInventorySummary();
    const FString Stats = GetStatsSummary(5);
    const FString Abil = GetAbilitiesSummary();

    UE_LOG(LogTemp, Log, TEXT("--- SOTS Suite State Dump ---"));
    UE_LOG(LogTemp, Log, TEXT("%s"), *GSM);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Mission);
    UE_LOG(LogTemp, Log, TEXT("%s"), *MMSS);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Tag);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Inv);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Stats);
    UE_LOG(LogTemp, Log, TEXT("%s"), *Abil);
    UE_LOG(LogTemp, Log, TEXT("----------------------------"));
}

void USOTS_SuiteDebugSubsystem::ToggleKEMAnchorOverlay()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    SetKEMAnchorOverlayEnabled(!IsKEMAnchorOverlayVisible());
#endif
}

void USOTS_SuiteDebugSubsystem::SetKEMAnchorOverlayEnabled(bool bEnabled)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!AreDebugWidgetsEnabled())
    {
        HideKEMAnchorOverlay();
        return;
    }

    if (bEnabled)
    {
        ShowKEMAnchorOverlay();
    }
    else
    {
        HideKEMAnchorOverlay();
    }
#endif
}

bool USOTS_SuiteDebugSubsystem::IsKEMAnchorOverlayVisible() const
{
    return KEMAnchorDebugWidgetInstance.IsValid();
}

void USOTS_SuiteDebugSubsystem::ShowKEMAnchorOverlay()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (IsKEMAnchorOverlayVisible())
    {
        return;
    }

    if (!AreDebugWidgetsEnabled())
    {
        return;
    }

    if (!KEMAnchorDebugWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("SuiteDebug: KEM anchor debug widget class is not configured."));
        return;
    }

    if (!GetWorld())
    {
        return;
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (USOTS_KillExecutionManagerKEMAnchorDebugWidget* Widget = CreateWidget<USOTS_KillExecutionManagerKEMAnchorDebugWidget>(PC, KEMAnchorDebugWidgetClass))
        {
            // TODO: route this through USOTS_UIRouterSubsystem so UI ownership stays centralized.
            Widget->AddToViewport();
            KEMAnchorDebugWidgetInstance = Widget;
        }
    }
#endif
}

void USOTS_SuiteDebugSubsystem::HideKEMAnchorOverlay()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (USOTS_KillExecutionManagerKEMAnchorDebugWidget* Widget = KEMAnchorDebugWidgetInstance.Get())
    {
        Widget->RemoveFromParent();
        KEMAnchorDebugWidgetInstance.Reset();
    }
#endif
}
