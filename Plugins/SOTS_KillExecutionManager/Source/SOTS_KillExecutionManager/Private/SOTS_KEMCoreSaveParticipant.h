#pragma once

#include "CoreMinimal.h"
#include "Save/SOTS_CoreSaveParticipant.h"

class FKEM_SaveParticipant final : public ISOTS_CoreSaveParticipant
{
public:
    virtual FName GetParticipantId() const override;
    virtual FSOTS_SaveParticipantStatus QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const override;
};
