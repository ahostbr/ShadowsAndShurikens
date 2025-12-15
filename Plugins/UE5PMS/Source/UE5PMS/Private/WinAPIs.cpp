//
// Copyright (c) 2021 - 2025 Advanced Micro Devices, Inc. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
//This abstracts Win32 APIs in ADLX ones so we insulate from platform
#include "ADLXDefines.h"
#include <cstdlib>

#include "HAL/PlatformAtomics.h"

#if defined(_WIN32) // Microsoft compiler
#include "Windows/MinWindows.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#else
#error define your copiler
#endif
static volatile uint64_t v = 0;

//----------------------------------------------------------------------------------------
// threading
//----------------------------------------------------------------------------------------
adlx_long ADLX_CDECL_CALL adlx_atomic_inc (adlx_long* X)
{
#if defined(_WIN32) // Microsoft compiler
    return static_cast<adlx_long>(FPlatformAtomics::InterlockedIncrement(reinterpret_cast<volatile int32*>(X)));
#endif
}

//----------------------------------------------------------------------------------------
adlx_long ADLX_CDECL_CALL adlx_atomic_dec (adlx_long* X)
{
#if defined(_WIN32) // Microsoft compiler
    return static_cast<adlx_long>(FPlatformAtomics::InterlockedDecrement(reinterpret_cast<volatile int32*>(X)));
#endif
}

//----------------------------------------------------------------------------------------
adlx_handle ADLX_CDECL_CALL adlx_load_library (const TCHAR* filename)
{
#if defined(METRO_APP)
    return LoadPackagedLibrary (filename, 0);
#elif defined(_WIN32) // Microsoft compiler
    return ::LoadLibraryEx (filename, nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS |
        LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
        LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
}

//----------------------------------------------------------------------------------------
int ADLX_CDECL_CALL adlx_free_library (adlx_handle module)
{
#if defined(_WIN32) // Microsoft compiler
    return ::FreeLibrary (reinterpret_cast<HMODULE>(module)) ? 1 : 0;
#endif
}

//----------------------------------------------------------------------------------------
void* ADLX_CDECL_CALL adlx_get_proc_address (adlx_handle module, const char* procName)
{
#if defined(_WIN32) // Microsoft compiler
    return (void*)::GetProcAddress ((HMODULE)module, procName);
#endif
}

//#endif //_WIN32