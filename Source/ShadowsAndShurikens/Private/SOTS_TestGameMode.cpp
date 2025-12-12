#include "SOTS_TestGameMode.h"

#include "SOTS_SuiteDebugSubsystem.h"

ASOTS_TestGameMode::ASOTS_TestGameMode()
{
    GameStateClass = ASOTS_TestGameState::StaticClass();
}

void ASOTS_TestGameMode::StartPlay()
{
    Super::StartPlay();
    LogSuiteStateOnStart();
}

void ASOTS_TestGameMode::LogSuiteStateOnStart()
{
    if (USOTS_SuiteDebugSubsystem* DebugSubsystem = USOTS_SuiteDebugSubsystem::Get(this))
    {
        UE_LOG(LogTemp, Log, TEXT("==== SOTS TestGameMode Start ===="));
        DebugSubsystem->DumpSuiteStateToLog();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_TestGameMode: Debug subsystem unavailable at StartPlay."));
    }
}
