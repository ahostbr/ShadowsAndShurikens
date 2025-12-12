// Copyright (c) 2025 USP45Master. All Rights Reserved.

#include "OmniTracePresetLibrary.h"
#include "OmniTracePatternPreset.h"

UOmniTracePatternPreset* UOmniTracePatternLibrary::FindPresetById(FName InPresetId) const
{
    if (InPresetId.IsNone())
    {
        return nullptr;
    }

    for (const FOmniTracePatternLibraryEntry& Entry : Entries)
    {
        if (!Entry.Preset)
        {
            continue;
        }

        const FName EffectiveId = Entry.PresetId.IsNone()
            ? Entry.Preset->PresetId
            : Entry.PresetId;

        if (EffectiveId == InPresetId)
        {
            return Entry.Preset;
        }
    }

    return nullptr;
}
