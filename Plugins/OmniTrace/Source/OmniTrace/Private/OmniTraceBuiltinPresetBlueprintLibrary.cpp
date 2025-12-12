// Copyright (c) 2025 USP45Master. All Rights Reserved.

#include "OmniTraceBlueprintLibrary.h"

#include "OmniTracePatternPreset.h"
#include "OmniTracePresetLibrary.h"

//////////////////////////////////////////////////////////////////////////
// Builtin preset registry & helpers (split from OmniTraceBlueprintLibrary)
//////////////////////////////////////////////////////////////////////////

namespace
{
/** Builtin preset definition. */
    struct FBuiltinOmniTracePreset
    {
        FName PresetId;
        FText DisplayName;
        FText Description;
        FName Category;
        FLinearColor FamilyColor;
        FOmniTraceRequest Request;
    };

    static TArray<FBuiltinOmniTracePreset>& GetMutableBuiltinPresets()
    {
        static TArray<FBuiltinOmniTracePreset> Presets;
        return Presets;
    }

    
static const TArray<FBuiltinOmniTracePreset>& GetBuiltinPresets()
    {
        TArray<FBuiltinOmniTracePreset>& Presets = GetMutableBuiltinPresets();
        if (Presets.Num() == 0)
        {
            // ------------------------------------------------------------------
            // Built-in presets used by the key-based enum selectors.
            // You can expand or tweak these freely – just keep PresetId stable
            // if you want existing Blueprint references to remain valid.
            // ------------------------------------------------------------------

            // --------------------
            // VISION / FOV
            // --------------------

            // Forward Cone – Close (5R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Fwd_Cone_Close_5R");
                Def.DisplayName = FText::FromString(TEXT("Forward Cone – Close (5R)"));
                Def.Description = FText::FromString(TEXT("Narrow forward cone, 5 rays, 1000 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 1000.0f;
                Req.RayCount           = 5;
                Req.SpreadAngleDegrees = 30.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.25f;
                Req.DebugOptions.Thickness    = 1.8f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Forward Cone – Medium (11R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Fwd_Cone_Medium");
                Def.DisplayName = FText::FromString(TEXT("Forward Cone – Medium (11R)"));
                Def.Description = FText::FromString(TEXT("45° cone, 11 rays, 2000 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 2000.0f;
                Req.RayCount           = 11;
                Req.SpreadAngleDegrees = 45.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.3f;
                Req.DebugOptions.Thickness    = 1.5f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Forward Cone – Long (21R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Fwd_Cone_Long_21R");
                Def.DisplayName = FText::FromString(TEXT("Forward Cone – Long (21R)"));
                Def.Description = FText::FromString(TEXT("60° cone, 21 rays, 3500 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 3500.0f;
                Req.RayCount           = 21;
                Req.SpreadAngleDegrees = 60.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.4f;
                Req.DebugOptions.Thickness    = 1.4f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Target Arc – Short (9R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Target_Arc_Short_9R");
                Def.DisplayName = FText::FromString(TEXT("Target Arc – Short (9R)"));
                Def.Description = FText::FromString(TEXT("Short arc fanned around the target direction, 9 rays, 1200 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily    = EOmniTraceTraceFamily::Target;
                Req.Shape            = EOmniTraceShape::Line;
                Req.TraceChannel     = ECC_Visibility;
                Req.MaxDistance      = 1200.0f;
                Req.RayCount         = 9;
                Req.ArcAngleDegrees  = 30.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.35f;
                Req.DebugOptions.Thickness    = 1.6f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Target Fan – Wide (21R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Target_Fan_Wide_21R");
                Def.DisplayName = FText::FromString(TEXT("Target Fan – Wide (21R)"));
                Def.Description = FText::FromString(TEXT("Wide arc centered on target, 21 rays, 2200 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily    = EOmniTraceTraceFamily::Target;
                Req.Shape            = EOmniTraceShape::Line;
                Req.TraceChannel     = ECC_Visibility;
                Req.MaxDistance      = 2200.0f;
                Req.RayCount         = 21;
                Req.ArcAngleDegrees  = 75.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.4f;
                Req.DebugOptions.Thickness    = 1.4f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // --------------------
            // COVERAGE / AREA
            // --------------------

            // Radial Burst – Sparse (64R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Radial_Burst_Sparse_64R");
                Def.DisplayName = FText::FromString(TEXT("Radial Burst – Sparse (64R)"));
                Def.Description = FText::FromString(TEXT("Lower cost radial coverage, 64 rays, 2000 units."));
                Def.Category    = TEXT("Coverage");
                Def.FamilyColor = FLinearColor::Yellow;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 2000.0f;
                Req.RayCount      = 64;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.6f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Radial Burst – Dense (128R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Radial_Burst_Dense_128R");
                Def.DisplayName = FText::FromString(TEXT("Radial Burst – Dense (128R)"));
                Def.Description = FText::FromString(TEXT("Balanced radial coverage, 128 rays, 2000 units."));
                Def.Category    = TEXT("Coverage");
                Def.FamilyColor = FLinearColor::Yellow;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 2000.0f;
                Req.RayCount      = 128;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.8f;
                Req.DebugOptions.Thickness    = 0.85f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Radial Burst – Very Dense (256R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Radial_Burst_Dense");
                Def.DisplayName = FText::FromString(TEXT("Radial Burst – Very Dense (256R)"));
                Def.Description = FText::FromString(TEXT("High quality radial coverage, 256 rays, 2000 units."));
                Def.Category    = TEXT("Coverage");
                Def.FamilyColor = FLinearColor::Yellow;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 2000.0f;
                Req.RayCount      = 256;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 0.8f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // --------------------
            // ORBIT / PERIMETER
            // --------------------

            // Orbit – Single Ring (24R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Orbit_Ring_24R");
                Def.DisplayName = FText::FromString(TEXT("Orbit – Single Ring (24R)"));
                Def.Description = FText::FromString(TEXT("Single horizontal ring around a center, 24 rays, 600 units."));
                Def.Category    = TEXT("Orbit");
                Def.FamilyColor = FLinearColor::Blue;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Orbit;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 600.0f;
                Req.RayCount      = 24;
                Req.OrbitRadius   = 300.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.6f;
                Req.DebugOptions.Thickness    = 1.0f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Orbit – Single Ring (48R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Orbit_Ring_48R");
                Def.DisplayName = FText::FromString(TEXT("Orbit – Single Ring (48R)"));
                Def.Description = FText::FromString(TEXT("Single horizontal ring around a center, 48 rays, 900 units."));
                Def.Category    = TEXT("Orbit");
                Def.FamilyColor = FLinearColor::Blue;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Orbit;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 900.0f;
                Req.RayCount      = 48;
                Req.OrbitRadius   = 450.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.8f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Orbit – Multi Ring (3 x 24R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Orbit_MultiRing_3x24R");
                Def.DisplayName = FText::FromString(TEXT("Orbit – Multi Ring (3 x 24R)"));
                Def.Description = FText::FromString(TEXT("Three stacked rings, 72 rays, 1000 units."));
                Def.Category    = TEXT("Orbit");
                Def.FamilyColor = FLinearColor::Blue;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Orbit;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 1000.0f;
                Req.RayCount      = 72;
                Req.OrbitRadius   = 500.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // --------------------
            // DEBUG / UTILITY
            // --------------------

            // Debug – Forward Long Ray
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Debug_Fwd_LongRay");
                Def.DisplayName = FText::FromString(TEXT("Debug – Forward Long Ray"));
                Def.Description = FText::FromString(TEXT("Single forward ray, 1 ray, 5000 units."));
                Def.Category    = TEXT("Debug");
                Def.FamilyColor = FLinearColor::Red;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::SingleRay;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 5000.0f;
                Req.RayCount           = 1;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.5f;
                Req.DebugOptions.Thickness    = 2.0f;

                Def.Request = Req;
                Presets.Add(Def);
            }

            // Debug – Wide Cone (9R)
            {
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Debug_WideCone_9R");
                Def.DisplayName = FText::FromString(TEXT("Debug – Wide Cone (9R)"));
                Def.Description = FText::FromString(TEXT("Wide cone for quick visual checks, 9 rays, 2000 units."));
                Def.Category    = TEXT("Debug");
                Def.FamilyColor = FLinearColor::Red;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 2000.0f;
                Req.RayCount           = 9;
                Req.SpreadAngleDegrees = 75.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 1.6f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Debug_Fwd_ShortRay");
                Def.DisplayName = FText::FromString(TEXT("Debug – Forward Short Ray"));
                Def.Description = FText::FromString(TEXT("Short single forward ray, 1 ray, 1000 units."));
                Def.Category    = TEXT("Debug");
                Def.FamilyColor = FLinearColor::Red;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::SingleRay;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 1000.0f;
                Req.RayCount           = 1;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.75f;
                Req.DebugOptions.Thickness    = 1.2f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Debug_Radial_32R");
                Def.DisplayName = FText::FromString(TEXT("Debug – Radial Burst (32R)"));
                Def.Description = FText::FromString(TEXT("Radial debug burst around the origin, 32 rays, 1200 units."));
                Def.Category    = TEXT("Debug");
                Def.FamilyColor = FLinearColor::Red;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 1200.0f;
                Req.RayCount      = 32;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 1.4f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Fwd_Cone_UltraWide_27R");
                Def.DisplayName = FText::FromString(TEXT("Forward Cone – Ultra Wide (27R)"));
                Def.Description = FText::FromString(TEXT("Very wide forward cone, 27 rays, 90° spread, 2500 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily      = EOmniTraceTraceFamily::Forward;
                Req.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
                Req.Shape              = EOmniTraceShape::Line;
                Req.TraceChannel       = ECC_Visibility;
                Req.MaxDistance        = 2500.0f;
                Req.RayCount           = 27;
                Req.SpreadAngleDegrees = 90.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.45f;
                Req.DebugOptions.Thickness    = 1.2f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Orbit_CloseGuard_16R");
                Def.DisplayName = FText::FromString(TEXT("Orbit – Close Guard (16R)"));
                Def.Description = FText::FromString(TEXT("Tight defensive ring near the origin, 16 rays, 700 units."));
                Def.Category    = TEXT("Orbit");
                Def.FamilyColor = FLinearColor::Blue;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Orbit;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 700.0f;
                Req.RayCount      = 16;
                Req.OrbitRadius   = 200.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.7f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Orbit_FarPerimeter_64R");
                Def.DisplayName = FText::FromString(TEXT("Orbit – Far Perimeter (64R)"));
                Def.Description = FText::FromString(TEXT("Large perimeter ring around an area, 64 rays, 1500 units."));
                Def.Category    = TEXT("Orbit");
                Def.FamilyColor = FLinearColor::Blue;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Orbit;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 1500.0f;
                Req.RayCount      = 64;
                Req.OrbitRadius   = 800.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Radial_Burst_Close_32R");
                Def.DisplayName = FText::FromString(TEXT("Radial Burst – Close (32R)"));
                Def.Description = FText::FromString(TEXT("Tight radial burst around the origin, 32 rays, 1000 units."));
                Def.Category    = TEXT("Coverage");
                Def.FamilyColor = FLinearColor::Yellow;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 1000.0f;
                Req.RayCount      = 32;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.5f;
                Req.DebugOptions.Thickness    = 0.9f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Radial_Burst_Far_128R");
                Def.DisplayName = FText::FromString(TEXT("Radial Burst – Far (128R)"));
                Def.Description = FText::FromString(TEXT("Far-reaching radial burst, 128 rays, 4000 units."));
                Def.Category    = TEXT("Coverage");
                Def.FamilyColor = FLinearColor::Yellow;

                FOmniTraceRequest Req{};
                Req.PatternFamily = EOmniTraceTraceFamily::Radial3D;
                Req.Shape         = EOmniTraceShape::Line;
                Req.TraceChannel  = ECC_Visibility;
                Req.MaxDistance   = 4000.0f;
                Req.RayCount      = 128;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 1.0f;
                Req.DebugOptions.Thickness    = 0.75f;

                Def.Request = Req;
                Presets.Add(Def);
            }

{
                FBuiltinOmniTracePreset Def;
                Def.PresetId    = TEXT("Target_Arc_Long_17R");
                Def.DisplayName = FText::FromString(TEXT("Target Vertical Arc – Long (17R)"));
                Def.Description = FText::FromString(TEXT("Long vertical arc around the target direction, 17 rays, 2500 units."));
                Def.Category    = TEXT("Vision");
                Def.FamilyColor = FLinearColor::Green;

                FOmniTraceRequest Req{};
                Req.PatternFamily    = EOmniTraceTraceFamily::Target;
                Req.Shape            = EOmniTraceShape::Line;
                Req.TraceChannel     = ECC_Visibility;
                Req.MaxDistance      = 2500.0f;
                Req.RayCount         = 17;
                Req.ArcAngleDegrees  = 60.0f;

                Req.DebugOptions.bEnableDebug = true;
                Req.DebugOptions.Color        = Def.FamilyColor;
                Req.DebugOptions.Lifetime     = 0.5f;
                Req.DebugOptions.Thickness    = 1.0f;

                Def.Request = Req;
                Presets.Add(Def);
            }


        }

        return Presets;
    }
static const FBuiltinOmniTracePreset* FindBuiltinPreset(FName InPresetId)
    {
        if (InPresetId.IsNone())
        {
            return nullptr;
        }

        const TArray<FBuiltinOmniTracePreset>& Presets = GetBuiltinPresets();
        for (const FBuiltinOmniTracePreset& P : Presets)
        {
            if (P.PresetId == InPresetId)
            {
                return &P;
            }
        }
        return nullptr;
    }

static FName MakeBuiltinPresetIdFromKey(const FOmniTraceBuiltinPresetKey& Key)
    {
        switch (Key.Category)
        {
        case EOmniTraceBuiltinPresetCategory::Vision:
            switch (Key.VisionPreset)
            {
            case EOmniTraceVisionBuiltinPreset::Fwd_Cone_Close_5R:        return TEXT("Fwd_Cone_Close_5R");
            case EOmniTraceVisionBuiltinPreset::Fwd_Cone_Medium_11R:      return TEXT("Fwd_Cone_Medium");
            case EOmniTraceVisionBuiltinPreset::Fwd_Cone_Long_21R:        return TEXT("Fwd_Cone_Long_21R");
            case EOmniTraceVisionBuiltinPreset::Target_Arc_Short_9R:      return TEXT("Target_Arc_Short_9R");
            case EOmniTraceVisionBuiltinPreset::Target_Fan_Wide_21R:      return TEXT("Target_Fan_Wide_21R");
            default:
                break;
            }
            break;

        case EOmniTraceBuiltinPresetCategory::Coverage:
            switch (Key.CoveragePreset)
            {
            case EOmniTraceCoverageBuiltinPreset::Radial_Sparse_64R:        return TEXT("Radial_Burst_Sparse_64R");
            case EOmniTraceCoverageBuiltinPreset::Radial_Dense_128R:        return TEXT("Radial_Burst_Dense_128R");
            case EOmniTraceCoverageBuiltinPreset::Radial_VeryDense_256R:    return TEXT("Radial_Burst_Dense");
            default:
                break;
            }
            break;

        case EOmniTraceBuiltinPresetCategory::Orbit:
            switch (Key.OrbitPreset)
            {
            case EOmniTraceOrbitBuiltinPreset::Orbit_SingleRing_24R:        return TEXT("Orbit_Ring_24R");
            case EOmniTraceOrbitBuiltinPreset::Orbit_SingleRing_48R:        return TEXT("Orbit_Ring_48R");
            case EOmniTraceOrbitBuiltinPreset::Orbit_MultiRing_3x24R:       return TEXT("Orbit_MultiRing_3x24R");
            default:
                break;
            }
            break;

        case EOmniTraceBuiltinPresetCategory::Debug:
            switch (Key.DebugPreset)
            {
            case EOmniTraceDebugBuiltinPreset::Debug_Fwd_LongRay:           return TEXT("Debug_Fwd_LongRay");
            case EOmniTraceDebugBuiltinPreset::Debug_Fwd_WideCone_9R:       return TEXT("Debug_WideCone_9R");
            default:
                break;
            }
            break;

        default:
            break;
        }

        return NAME_None;
    }
}

FOmniTracePatternResult UOmniTraceBlueprintLibrary::OmniTrace_PatternFromBuiltinPreset(
    UObject* WorldContextObject,
    FName PresetId,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    FOmniTracePatternResult Result;

    if (!WorldContextObject)
    {
        return Result;
    }

    const FBuiltinOmniTracePreset* Def = FindBuiltinPreset(PresetId);
    if (!Def)
    {
        return Result;
    }

    FOmniTraceRequest Request = Def->Request;

    // Sync debug color with FamilyColor so visual grouping stays consistent.
    Request.DebugOptions.Color = Def->FamilyColor;

    if (OriginActorOverride)
    {
        Request.OriginActor = OriginActorOverride;
    }

    if (OriginComponentOverride)
    {
        Request.OriginComponent = OriginComponentOverride;
    }

    if (TargetActorOverride)
    {
        Request.TargetActor = TargetActorOverride;
    }

    return OmniTrace_Pattern(WorldContextObject, Request);
}

void UOmniTraceBlueprintLibrary::OmniTrace_PatternFromBuiltinPreset_Async(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FOmniTracePatternResult& OutResult,
    FName PresetId,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    if (!WorldContextObject)
    {
        return;
    }

    const FBuiltinOmniTracePreset* Def = FindBuiltinPreset(PresetId);
    if (!Def)
    {
        return;
    }

    FOmniTraceRequest Request = Def->Request;
    Request.DebugOptions.Color = Def->FamilyColor;

    if (OriginActorOverride)
    {
        Request.OriginActor = OriginActorOverride;
    }

    if (OriginComponentOverride)
    {
        Request.OriginComponent = OriginComponentOverride;
    }

    if (TargetActorOverride)
    {
        Request.TargetActor = TargetActorOverride;
    }

    OmniTrace_Pattern_Async(WorldContextObject, LatentInfo, OutResult, Request);
}

FName UOmniTraceBlueprintLibrary::OmniTrace_MakeBuiltinPresetId(const FOmniTraceBuiltinPresetKey& Key)
{
    return MakeBuiltinPresetIdFromKey(Key);
}

FOmniTracePatternResult UOmniTraceBlueprintLibrary::OmniTrace_PatternFromBuiltinPresetKey(
    UObject* WorldContextObject,
    const FOmniTraceBuiltinPresetKey& Key,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    const FName PresetId = MakeBuiltinPresetIdFromKey(Key);
    return OmniTrace_PatternFromBuiltinPreset(
        WorldContextObject,
        PresetId,
        OriginActorOverride,
        OriginComponentOverride,
        TargetActorOverride);
}

void UOmniTraceBlueprintLibrary::OmniTrace_PatternFromBuiltinPresetKey_Async(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FOmniTracePatternResult& OutResult,
    const FOmniTraceBuiltinPresetKey& Key,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    const FName PresetId = MakeBuiltinPresetIdFromKey(Key);
    OmniTrace_PatternFromBuiltinPreset_Async(
        WorldContextObject,
        LatentInfo,
        OutResult,
        PresetId,
        OriginActorOverride,
        OriginComponentOverride,
        TargetActorOverride);
}

void UOmniTraceBlueprintLibrary::OmniTrace_GetBuiltinPresetInfos(
    TArray<FOmniTracePresetInfo>& OutInfos)
{
    OutInfos.Reset();

    const TArray<FBuiltinOmniTracePreset>& Presets = GetBuiltinPresets();
    OutInfos.Reserve(Presets.Num());

    for (const FBuiltinOmniTracePreset& Def : Presets)
    {
        FOmniTracePresetInfo Info;
        Info.PresetId    = Def.PresetId;
        Info.DisplayName = Def.DisplayName;
        Info.Description = Def.Description;
        Info.Category    = Def.Category;
        Info.FamilyColor = Def.FamilyColor;

        OutInfos.Add(Info);
    }
}

