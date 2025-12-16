#include "SOTS_UISettings.h"

USOTS_UISettings::USOTS_UISettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

FName USOTS_UISettings::GetCategoryName() const
{
    return FName(TEXT("SOTS"));
}

#if WITH_EDITOR
FText USOTS_UISettings::GetSectionText() const
{
    return NSLOCTEXT("SOTS_UI", "SettingsSection", "UI");
}
#endif

const USOTS_UISettings* USOTS_UISettings::Get()
{
    return GetDefault<USOTS_UISettings>();
}
