#include "SOTS_InputLayerDataAsset.h"

#include "InputAction.h"
#include "SOTS_InputHandler.h"

void USOTS_InputLayerDataAsset::GetAllBindings(TMap<const UInputAction*, TSet<ETriggerEvent>>& Out) const
{
    for (const USOTS_InputHandler* Handler : HandlerTemplates)
    {
        if (!Handler)
        {
            continue;
        }

        for (const UInputAction* Action : Handler->InputActions)
        {
            if (!Action)
            {
                continue;
            }

            TSet<ETriggerEvent>& Events = Out.FindOrAdd(Action);
            for (const ETriggerEvent Event : Handler->TriggerEvents)
            {
                Events.Add(Event);
            }
        }
    }
}
