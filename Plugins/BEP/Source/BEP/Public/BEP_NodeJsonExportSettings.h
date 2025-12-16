#pragma once

#include "CoreMinimal.h"
#include "BEP_NodeJsonExport.h"
#include "BEP_NodeJsonExportSettings.generated.h"

/** Editor per-project settings for BEP node JSON export. */
UCLASS(config=EditorPerProjectUserSettings)
class BEP_API UBEP_NodeJsonExportSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ToolTip="Default preset applied to exports when opening the panel or using menu commands."))
    EBEP_NodeJsonPreset DefaultPreset = EBEP_NodeJsonPreset::SuperCompact;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ClampMin="0", ClampMax="10", ToolTip="Exec neighbor expansion depth (0 = selection only)."))
    int32 ExecDepth = 0;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ClampMin="0", ClampMax="10", ToolTip="Data neighbor expansion depth (0 = selection only)."))
    int32 DataDepth = 0;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bPrettyPrint = false;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bCompactKeys = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeLegend = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeClassDict = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludePosition = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludePinDecls = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeFullPins = false;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeUnconnectedPins = false;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeExecEdges = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeDataEdges = true;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeNodeTitle = false;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON")
    bool bIncludeNodeComment = false;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ClampMin="1", ClampMax="5000", ToolTip="Hard cap on nodes after expansion; clamped deterministically when exceeded."))
    int32 MaxNodesHardCap = 500;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ClampMin="1", ClampMax="10000", ToolTip="Hard cap on edges after rebuilding within kept nodes."))
    int32 MaxEdgesHardCap = 2000;

    UPROPERTY(config, EditAnywhere, Category="BEP|NodeJSON", meta=(ToolTip="If true, warn when caps are hit before clamping."))
    bool bWarnOnCapHit = true;

    /** Apply the selected preset directly into this settings object. */
    void ApplyPresetToSettings(EBEP_NodeJsonPreset Preset);

    /** Build export options from the current settings state. */
    void BuildExportOptions(FBEP_NodeJsonExportOptions& OutOptions) const;
};
