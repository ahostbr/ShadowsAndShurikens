#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "Save/SOTS_SaveTypes.h"

static const FName SOTS_CoreSaveParticipantFeatureName(TEXT("SOTS.CoreSaveParticipant"));

class ISOTS_CoreSaveParticipant : public IModularFeature
{
public:
    virtual ~ISOTS_CoreSaveParticipant() = default;

    virtual FName GetParticipantId() const = 0;

    virtual FSOTS_SaveParticipantStatus QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const
    {
        return FSOTS_SaveParticipantStatus();
    }

    virtual bool BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out)
    {
        return false;
    }

    virtual bool ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In)
    {
        return false;
    }
};
