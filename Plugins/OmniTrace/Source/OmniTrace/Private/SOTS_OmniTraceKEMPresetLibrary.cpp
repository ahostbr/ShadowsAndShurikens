#include "SOTS_OmniTraceKEMPresetLibrary.h"

#include "OmniTracePatternPreset.h"

const FSOTS_OmniTraceKEMPresetEntry* USOTS_OmniTraceKEMPresetLibrary::FindEntry(ESOTS_OmniTraceKEMPreset Preset) const
{
    if (Preset == ESOTS_OmniTraceKEMPreset::Unknown)
    {
        return nullptr;
    }

    for (const FSOTS_OmniTraceKEMPresetEntry& Entry : Entries)
    {
        if (Entry.PresetId == Preset)
        {
            return &Entry;
        }
    }

    return nullptr;
}

UOmniTracePatternPreset* USOTS_OmniTraceKEMPresetLibrary::FindPreset(ESOTS_OmniTraceKEMPreset Preset) const
{
    if (const FSOTS_OmniTraceKEMPresetEntry* Entry = FindEntry(Preset))
    {
        return Entry->PatternPreset.LoadSynchronous();
    }
    return nullptr;
}
