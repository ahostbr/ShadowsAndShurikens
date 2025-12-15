#include "ContextualAnimSceneAsset.h"
#include "SOTS_KEM_ExecutionDefinition.h"

FSOTS_KEMValidationResult USOTS_KEM_ExecutionDefinition::ValidateDefinition() const
{
    FSOTS_KEMValidationResult Result;

    // DevTools: Python coverage tools can call ValidateDefinition() indirectly
    // via logs or debug commands, so keep messages deterministic and short.

    if (ExecutionFamily == ESOTS_KEM_ExecutionFamily::Unknown)
    {
        Result.AddWarning(TEXT("ExecutionFamily is Unknown."));
    }
    if (!PositionTag.IsValid())
    {
        Result.AddWarning(TEXT("PositionTag is not set."));
    }

    if (!ExecutionTag.IsValid())
    {
        Result.AddWarning(TEXT("ExecutionTag is not set."));
    }

    if (RequiredContextTags.IsEmpty() && BlockedContextTags.IsEmpty())
    {
        Result.AddWarning(TEXT("No context tags configured."));
    }

    switch (BackendType)
    {
    case ESOTS_KEM_BackendType::SpawnActor:
        if (WarpPoints.Num() == 0)
        {
            Result.AddWarning(TEXT("SpawnActor backend has no warp points configured."));
        }

        if (!SpawnActorConfig.ExecutionData.IsValid() &&
            !SpawnActorConfig.ExecutionData.ToSoftObjectPath().IsValid())
        {
            Result.AddError(TEXT("SpawnActor backend requires ExecutionData."));
        }

        if (!SpawnActorConfig.HelperClass)
        {
            Result.AddWarning(TEXT("SpawnActor backend has no HelperClass."));
        }

        if (SpawnActorConfig.ExecutionData.IsValid())
        {
            if (const USOTS_SpawnExecutionData* SpawnData = SpawnActorConfig.ExecutionData.Get())
            {
                if (SpawnData->InstigatorWarpPointNames.Num() == 0)
                {
                    Result.AddWarning(TEXT("SpawnExecutionData lacks instigator warp points."));
                }

                if (!SpawnData->InstigatorMontage && !SpawnData->TargetMontage)
                {
                    Result.AddWarning(TEXT("SpawnExecutionData has no montages configured."));
                }
            }
        }
        break;

    case ESOTS_KEM_BackendType::CAS:
        if (CASConfig.MinDistance > CASConfig.MaxDistance)
        {
            Result.AddError(TEXT("CAS MinDistance > MaxDistance."));
        }

        if (!CASConfig.Scene.IsValid() && !CASConfig.Scene.ToSoftObjectPath().IsValid())
        {
            Result.AddWarning(TEXT("CAS Scene asset is not configured."));
        }

        if (CASConfig.SectionName.IsNone())
        {
            Result.AddWarning(TEXT("CAS SectionName is not set."));
        }
        break;

    case ESOTS_KEM_BackendType::LevelSequence:
        if (!LevelSequenceConfig.SequenceAsset.IsValid() &&
            !LevelSequenceConfig.SequenceAsset.ToSoftObjectPath().IsValid())
        {
            Result.AddWarning(TEXT("LevelSequence backend has no SequenceAsset."));
        }
        break;

    case ESOTS_KEM_BackendType::AIS:
        Result.AddError(TEXT("AIS backend is retired and not supported."));
        break;

    default:
        break;
    }

    return Result;
}

ESOTS_KEM_PositionKind USOTS_KEM_ExecutionDefinition::GetPositionKind() const
{
    if (!PositionTag.IsValid())
    {
        return ESOTS_KEM_PositionKind::Unknown;
    }

    const FName TagName = PositionTag.GetTagName();

    if (TagName == TEXT("SOTS.KEM.Position.Ground.Rear"))
    {
        return ESOTS_KEM_PositionKind::GroundRear;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Ground.Front"))
    {
        return ESOTS_KEM_PositionKind::GroundFront;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Ground.Left"))
    {
        return ESOTS_KEM_PositionKind::GroundLeft;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Ground.Right"))
    {
        return ESOTS_KEM_PositionKind::GroundRight;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Vertical.Above"))
    {
        return ESOTS_KEM_PositionKind::VerticalAbove;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Vertical.Below"))
    {
        return ESOTS_KEM_PositionKind::VerticalBelow;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Corner.Left"))
    {
        return ESOTS_KEM_PositionKind::CornerLeft;
    }
    if (TagName == TEXT("SOTS.KEM.Position.Corner.Right"))
    {
        return ESOTS_KEM_PositionKind::CornerRight;
    }

    return ESOTS_KEM_PositionKind::Unknown;
}
