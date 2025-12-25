#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SOTS_PlayerCharacterBase.generated.h"

UCLASS()
class SOTS_CORE_API ASOTS_PlayerCharacterBase : public ACharacter
{
    GENERATED_BODY()

protected:
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;
};
