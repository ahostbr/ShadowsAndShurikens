// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Settings/NinjaInputPlayerMappableKeySettings.h"

const FGameplayTag& UNinjaInputPlayerMappableKeySettings::GetInputFilter() const
{
	return InputFilter;
}

const FText& UNinjaInputPlayerMappableKeySettings::GetTooltipText() const
{
	return Tooltip;
}
