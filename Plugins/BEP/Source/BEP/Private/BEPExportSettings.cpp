#include "BEPExportSettings.h"

#include "Misc/Paths.h"

BEP_API FString GetDefaultBEPExportRoot()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("BEP_EXPORTS"));
}
