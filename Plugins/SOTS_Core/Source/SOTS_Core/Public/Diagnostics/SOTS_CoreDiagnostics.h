#pragma once

#include "CoreMinimal.h"

class UWorld;

class SOTS_CORE_API FSOTS_CoreDiagnostics
{
public:
    static void DumpSettings();
    static void DumpLifecycleSnapshot(UWorld* World);
    static void DumpRegisteredLifecycleListeners();
    static void DumpRegisteredSaveParticipants();
    static void PrintHealthReport(UWorld* World);
    static void ValidateCoreConfiguration(UWorld* World);

private:
    static int32 GetRegisteredLifecycleListenerCount();
    static int32 GetRegisteredSaveParticipantCount();
};
