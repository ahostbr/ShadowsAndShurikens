#pragma once

#include "CoreMinimal.h"
#include "Handlers/SOTS_InputHandler_IntentTag.h"
#include "SOTS_InputHandler_MenuBack.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SOTS_INPUT_API USOTS_InputHandler_MenuBack : public USOTS_InputHandler_IntentTag
{
    GENERATED_BODY()

public:
    USOTS_InputHandler_MenuBack();
};
