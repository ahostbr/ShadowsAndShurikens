#include "Handlers/SOTS_InputHandler_MenuBack.h"

USOTS_InputHandler_MenuBack::USOTS_InputHandler_MenuBack()
{
    IntentTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Intent.UI.Back"), false);
}
