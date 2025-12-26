#pragma once

#include "CoreMinimal.h"
#include "Save/SOTS_CoreSaveParticipant.h"

class FINV_SaveParticipant final : public ISOTS_CoreSaveParticipant
{
public:
    virtual FName GetParticipantId() const override;

    virtual FSOTS_SaveParticipantStatus QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const override;

    virtual bool BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out) override;

    virtual bool ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In) override;
};
