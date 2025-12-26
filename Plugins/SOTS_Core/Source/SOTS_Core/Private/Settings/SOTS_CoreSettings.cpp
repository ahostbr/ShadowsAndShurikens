#include "Settings/SOTS_CoreSettings.h"
#include "SOTS_Core.h"

void USOTS_CoreSettings::PostInitProperties()
{
    Super::PostInitProperties();
    ApplyConfigMigrations();
}

void USOTS_CoreSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
    Super::PostReloadConfig(PropertyThatWasLoaded);
    ApplyConfigMigrations();
}

void USOTS_CoreSettings::ApplyConfigMigrations()
{
    const int32 CurrentSchema = SOTS_CORE_CONFIG_SCHEMA_VERSION;
    const int32 PreviousSchema = ConfigSchemaVersion;

    if (PreviousSchema == CurrentSchema)
    {
        return;
    }

    if (PreviousSchema <= 0)
    {
        ConfigSchemaVersion = CurrentSchema;

        if (bEnableVerboseCoreLogs)
        {
            UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core settings schema reset to %d (previous=%d)."), CurrentSchema, PreviousSchema);
        }
        else
        {
            UE_LOG(LogSOTS_Core, Verbose, TEXT("SOTS_Core settings schema reset to %d (previous=%d)."), CurrentSchema, PreviousSchema);
        }

        return;
    }

    if (PreviousSchema < CurrentSchema)
    {
        // No field migrations yet; bump schema to current.
        ConfigSchemaVersion = CurrentSchema;

        if (bEnableVerboseCoreLogs)
        {
            UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core settings schema upgraded from %d to %d."), PreviousSchema, CurrentSchema);
        }
        else
        {
            UE_LOG(LogSOTS_Core, Verbose, TEXT("SOTS_Core settings schema upgraded from %d to %d."), PreviousSchema, CurrentSchema);
        }

        return;
    }

    UE_LOG(
        LogSOTS_Core,
        Warning,
        TEXT("SOTS_Core settings schema version %d is newer than supported %d; leaving values unchanged."),
        PreviousSchema,
        CurrentSchema);
}

FName USOTS_CoreSettings::GetCategoryName() const
{
    return TEXT("SOTS");
}

FName USOTS_CoreSettings::GetSectionName() const
{
    return TEXT("Core");
}
