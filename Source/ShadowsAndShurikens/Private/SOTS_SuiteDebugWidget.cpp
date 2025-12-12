// SPDX-License-Identifier: MIT
#include "SOTS_SuiteDebugWidget.h"
#include "SOTS_SuiteDebugSubsystem.h"

FString USOTS_SuiteDebugWidget::GetGSMStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetGlobalStealthSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetPlayerStatsText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetStatsSummary(5);
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetAbilityStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetAbilitiesSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetInventoryStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetInventorySummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetMissionStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetMissionDirectorSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetMusicStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetMusicSubsystemSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetFXStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetFXSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

FString USOTS_SuiteDebugWidget::GetTagManagerStateText() const
{
    if (const USOTS_SuiteDebugSubsystem* Sub = GetSuiteDebugSubsystem())
    {
        return Sub->GetTagManagerSummary();
    }
    return TEXT("DebugSubsystem: <Unavailable>");
}

USOTS_SuiteDebugSubsystem* USOTS_SuiteDebugWidget::GetSuiteDebugSubsystem() const
{
    return USOTS_SuiteDebugSubsystem::Get(this);
}
