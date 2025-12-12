// SOTS_TestActor.cpp

#include "SOTS_TestActor.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTSTestActor, Log, All);

ASOTS_TestActor::ASOTS_TestActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Message = TEXT("SOTS Test Actor says: Kuroryuu pipeline online!");
    bPrintEveryTick = false;
}

void ASOTS_TestActor::BeginPlay()
{
    Super::BeginPlay();

    // Print to the output log
    UE_LOG(LogSOTSTestActor, Log, TEXT("%s"), *Message);

    // Also print on screen once so you can see it in PIE
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            5.0f,
            FColor::Green,
            Message
        );
    }
}

void ASOTS_TestActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bPrintEveryTick)
    {
        UE_LOG(LogSOTSTestActor, Verbose, TEXT("Tick: %s"), *Message);
    }
}
