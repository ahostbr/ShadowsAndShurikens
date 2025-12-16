#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EBEP_NodeJsonPreset : uint8
{
    AIMinimal UMETA(DisplayName = "AI Minimal"),
    Compact UMETA(DisplayName = "Compact"),
    SuperCompact UMETA(DisplayName = "Super Compact"),
    AIAudit UMETA(DisplayName = "AI Audit"),
    Custom UMETA(DisplayName = "Custom")
};

struct FBEP_NodeJsonExportOptions
{
    FString EngineTag = TEXT("UE5.7");
    EBEP_NodeJsonPreset Preset = EBEP_NodeJsonPreset::SuperCompact;

    bool bIncludeAssetPath     = false;
    bool bIncludeGraphName     = true;
    bool bIncludeNodeTitle     = false;
    bool bIncludeNodeComment   = false;

    bool bIncludePosition      = true;

    bool bIncludePinDecls      = true;  // pd
    bool bIncludeFullPins      = false; // p (audit)
    bool bIncludeUnconnectedPins = false; // pu (audit)
    bool bIncludeDefaultValues = true;
    bool bIncludePinFlags      = false;
    bool bIncludeClassDict     = true;

    bool bIncludeExecEdges     = true;
    bool bIncludeDataEdges     = true;

    int32 MaxNodesHardCap = 500;
    int32 MaxEdgesHardCap = 2000;
    bool bWarnOnCapHit = true;

    int32 ExecDepth = 0;  // neighbor expansion depth via exec connections
    int32 DataDepth = 0;  // neighbor expansion depth via data connections

    bool bPrettyPrint   = false;   // SuperCompact should be false
    bool bCompactKeys   = true;    // SuperCompact should be true
    bool bIncludeLegend = false;   // optional field legend
};

struct FBEP_NodeJsonExportStats
{
    int32 SeedSelectionCount = 0;
    int32 ExpandedNodeCount = 0;
    int32 FinalNodeCount = 0;
    int32 ExecEdgeCount = 0;
    int32 DataEdgeCount = 0;
    bool bHitNodeCap = false;
    bool bHitEdgeCap = false;
    int32 MaxNodeCap = 0;
    int32 MaxEdgeCap = 0;
    int32 ExecDepthUsed = 0;
    int32 DataDepthUsed = 0;
};

namespace BEP_NodeJson
{
    BEP_API void ApplyPreset(FBEP_NodeJsonExportOptions& Opt, EBEP_NodeJsonPreset Preset);

    /** Returns false if no active graph editor or no selected nodes. */
    BEP_API bool ExportActiveSelectionToJson(
        const FBEP_NodeJsonExportOptions& Opt,
        FString& OutJson,
        FString& OutSuggestedFileStem,
        int32& OutNodeCount,
        int32& OutEdgeCount,
        FString* OutError = nullptr,
        FBEP_NodeJsonExportStats* OutStats = nullptr,
        bool bPreviewOnly = false);

    /** Saves JSON to disk under the BEP export root. */
    BEP_API bool SaveJsonToFile(
        const FString& Json,
        const FString& SuggestedFileStem,
        const FBEP_NodeJsonExportOptions& Opt,
        const FBEP_NodeJsonExportStats* Stats,
        FString& OutAbsPath,
        FString* OutSummaryPath = nullptr,
        FString* OutError = nullptr,
        bool bGoldenSample = false);

    /** Export a lightweight “comment JSON” describing selected nodes for AI comment generation. */
    BEP_API bool ExportActiveSelectionToCommentJson(
        const FBEP_NodeJsonExportOptions& Opt,
        FString& OutJson,
        FString& OutSuggestedFileStem,
        TArray<FString>* OutGuids,
        FString* OutError = nullptr);

    /** Save the comment JSON to disk under the BEP export root. */
    BEP_API bool SaveCommentJsonToFile(
        const FString& Json,
        const FString& SuggestedFileStem,
        const FBEP_NodeJsonExportOptions& Opt,
        FString& OutAbsPath,
        FString* OutTemplateCsvPath = nullptr,
        FString* OutError = nullptr,
        const TArray<FString>* TemplateGuids = nullptr,
        bool bGoldenSample = false);

    /** Write fixed-name golden samples (SuperCompact, AIAudit, Comments) into the GoldenSamples folder. */
    BEP_API bool WriteGoldenSamples(
        FString& OutGoldenSummaryPath,
        FString* OutError = nullptr,
        FString* OutSuperCompactPath = nullptr,
        FString* OutAuditPath = nullptr,
        FString* OutCommentPath = nullptr,
        FString* OutTemplatePath = nullptr);

    /** Lightweight capability checks for menus/panels. */
    BEP_API bool HasActiveBlueprintGraph(FString* OutReason = nullptr);
    BEP_API bool HasActiveBlueprintSelection(FString* OutReason = nullptr);

    /** Diagnostic self-check; returns true if all checks pass and fills report. */
    BEP_API bool RunSelfCheck(FString& OutReport);
}
