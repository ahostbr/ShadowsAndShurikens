#include "SOTS_UISettings.h"

USOTS_UISettings::USOTS_UISettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

FName USOTS_UISettings::GetCategoryName() const
{
    return FName(TEXT("SOTS"));
}

FText USOTS_UISettings::GetSectionText() const
{
    return NSLOCTEXT("SOTS_UI", "SettingsSection", "UI");
}

const USOTS_UISettings* USOTS_UISettings::Get()
{
    return GetDefault<USOTS_UISettings>();
}
