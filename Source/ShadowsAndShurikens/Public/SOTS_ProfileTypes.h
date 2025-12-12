#pragma once

// Wrapper to reference the canonical profile structs in the SOTS_ProfileShared plugin.
// This intentionally includes the plugin header directly to avoid UHT duplicate-name issues.
// After you confirm the build, delete or rename this file so UHT only sees the plugin header.
#if __has_include("E:/SAS/ShadowsAndShurikens/Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h")
#include "E:/SAS/ShadowsAndShurikens/Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h"
#else
#error "Canonical SOTS_ProfileTypes.h not found in SOTS_ProfileShared plugin. Rename/delete the game copy and ensure the plugin is present."
#endif
