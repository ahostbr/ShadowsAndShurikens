#pragma once

#include "CoreMinimal.h"

// Global mutex to serialize BPGen edits across ensure/apply surfaces.
extern FCriticalSection GSOTS_BPGenEditMutex;
