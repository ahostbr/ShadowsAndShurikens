#include "SOTS_FXBlueprintLibrary.h"

#include "SOTS_FXManagerSubsystem.h"

#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "SOTS_FXCueDefinition.h"

USOTS_FXManagerSubsystem* USOTS_FXBlueprintLibrary::GetFXManager()
{
    return USOTS_FXManagerSubsystem::Get();
}

FSOTS_FXHandle USOTS_FXBlueprintLibrary::PlayFXCueByTag(FGameplayTag CueTag, const FSOTS_FXContext& Context)
{
    FSOTS_FXHandle Handle;

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        Handle = Manager->PlayCueByTag(CueTag, Context);
    }

    return Handle;
}

FSOTS_FXHandle USOTS_FXBlueprintLibrary::PlayFXCueDefinition(USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context)
{
    FSOTS_FXHandle Handle;

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        Handle = Manager->PlayCueDefinition(CueDefinition, Context);
    }

    return Handle;
}

FSOTS_FXRequestResult USOTS_FXBlueprintLibrary::TriggerFXWithReport(UObject* WorldContextObject, const FSOTS_FXRequest& Request)
{
    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        return Manager->TriggerFXByTagWithReport(WorldContextObject, Request);
    }

    FSOTS_FXRequestResult Result;
    Result.Status = ESOTS_FXRequestStatus::MissingContext;
    return Result;
}

FSOTS_FXRequestReport USOTS_FXBlueprintLibrary::RequestFXCue(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target)
{
    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        return Manager->RequestFXCueWithReport(FXCueTag, Instigator, Target);
    }

    FSOTS_FXRequestReport Report;
    Report.RequestedCueTag = FXCueTag;
    Report.ResolvedCueTag = FXCueTag;
    Report.Result = ESOTS_FXRequestResult::InvalidWorld;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    Report.DebugMessage = TEXT("FXManagerSubsystem missing");
#endif
    return Report;
}

void USOTS_FXBlueprintLibrary::StopFXHandle(const FSOTS_FXHandle& Handle, bool bImmediate)
{
    if (Handle.NiagaraComponent)
    {
        if (bImmediate)
        {
            Handle.NiagaraComponent->DeactivateImmediate();
        }
        else
        {
            Handle.NiagaraComponent->Deactivate();
        }
    }

    if (Handle.AudioComponent)
    {
        Handle.AudioComponent->Stop();
    }
}

FSOTS_FXActiveCounts USOTS_FXBlueprintLibrary::GetFXActiveCounts()
{
    FSOTS_FXActiveCounts Counts;

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        int32 ActiveNiagara = 0;
        int32 ActiveAudio = 0;
        Manager->GetActiveFXCountsInternal(ActiveNiagara, ActiveAudio);

        Counts.ActiveNiagara = ActiveNiagara;
        Counts.ActiveAudio = ActiveAudio;
    }

    return Counts;
}

FSOTS_FXPoolStats USOTS_FXBlueprintLibrary::GetFXPoolStats()
{
    FSOTS_FXPoolStats Stats;

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        Stats = Manager->GetPoolStats();
    }

    return Stats;
}

void USOTS_FXBlueprintLibrary::GetRegisteredFXCues(TArray<FGameplayTag>& OutCueTags, TArray<USOTS_FXCueDefinition*>& OutCueDefinitions)
{
    OutCueTags.Reset();
    OutCueDefinitions.Reset();

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        Manager->GetRegisteredCuesInternal(OutCueTags, OutCueDefinitions);
    }
}
