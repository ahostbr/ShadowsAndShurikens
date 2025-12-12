#include "SOTS_ParkourStatsWidget.h"

void USOTS_ParkourStatsWidget::SetParkourStateTag_Implementation(FGameplayTag StateTag)
{
    ParkourStateTag = StateTag;
}

void USOTS_ParkourStatsWidget::SetParkourActionTag_Implementation(FGameplayTag ActionTag)
{
    ParkourActionTag = ActionTag;
}

void USOTS_ParkourStatsWidget::SetParkourClimbStyleTag_Implementation(FGameplayTag ClimbStyleTag)
{
    ParkourClimbStyleTag = ClimbStyleTag;
}

FText USOTS_ParkourStatsWidget::GetParkourStateText() const
{
    return ToText(ParkourStateTag);
}

FText USOTS_ParkourStatsWidget::GetParkourActionText() const
{
    return ToText(ParkourActionTag);
}

FText USOTS_ParkourStatsWidget::GetParkourClimbStyleText() const
{
    return ToText(ParkourClimbStyleTag);
}

FText USOTS_ParkourStatsWidget::ToText(const FGameplayTag& Tag) const
{
    return Tag.IsValid() ? FText::FromName(Tag.GetTagName()) : FText();
}
