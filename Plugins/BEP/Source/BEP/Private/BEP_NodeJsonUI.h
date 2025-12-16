#pragma once

#include "CoreMinimal.h"

struct FBEP_NodeJsonExportOptions;
class UBEP_NodeJsonExportSettings;

namespace BEPNodeJsonUI
{
    /** Build export options from saved settings, falling back to SuperCompact if unavailable. */
    FBEP_NodeJsonExportOptions BuildOptionsFromSettings();

    /** Show a short toast about Node JSON operations. */
    void ShowNodeJsonToast(const FString& Message, bool bSuccess);
}
