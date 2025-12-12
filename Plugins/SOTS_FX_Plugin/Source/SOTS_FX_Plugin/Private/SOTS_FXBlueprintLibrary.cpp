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

void USOTS_FXBlueprintLibrary::GetRegisteredFXCues(TArray<FGameplayTag>& OutCueTags, TArray<USOTS_FXCueDefinition*>& OutCueDefinitions)
{
    OutCueTags.Reset();
    OutCueDefinitions.Reset();

    if (USOTS_FXManagerSubsystem* Manager = USOTS_FXManagerSubsystem::Get())
    {
        Manager->GetRegisteredCuesInternal(OutCueTags, OutCueDefinitions);
    }
}
