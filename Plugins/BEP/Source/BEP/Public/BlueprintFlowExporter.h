#pragma once

#include "CoreMinimal.h"
#include "BEPExportSettings.h"
#include "AssetRegistry/AssetData.h"

class UBlueprint;
class UEdGraph;
class UEdGraphPin;

/**
 * BEP - Blueprint & data exporter.
 *
 * All functions here are editor-only and intended to be used via console commands.
 */

enum class EBEPExportFormat : uint8
{
	Text,
	Json,
	Csv
};

class FBlueprintFlowExporter
{
public:
	/** Export all Blueprint graphs (flows) under the given root (default /Game). */
	static void ExportAllBlueprintFlows(const FString& OutputDirectory, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun = 0);

	/** Export all Input Mapping Contexts (IMCs) under the given root. */
	static void ExportAllInputMappingContexts(const FString& OutputDirectory, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun = 0);

	/** Export all DataAssets and DataTables under the given root (always JSON payload, regardless of format). */
	static void ExportAllDataAssetsAndTables(const FString& OutputDirectory, const FString& RootPath, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun = 0);

	/** Export struct schemas (UScriptStruct and UUserDefinedStruct) to a single text file. */
	static void ExportAllStructSchemas(const FString& OutputFilePath);

	/** Convenience function to export everything to Saved/BEPExport with given root and format. */
	static void ExportAll(const FString& OutputRoot, const FString& RootPath, EBEPExportFormat Format, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun = 0);

	/** High-level export entrypoint driven by UI settings. */
	static void ExportWithSettings(const FBEPExportSettings& Settings);

	/** Parse a format string ("text", "json", "csv") into EBEPExportFormat, defaulting to Text. */
	static EBEPExportFormat ParseFormat(const FString& InFormatString);

	/** Return the literal default value for this pin (object/text/value), or empty if none. */
	static FString GetPinDefaultValueString(UEdGraphPin* Pin);

	/**
	 * Describe the resolved value for this pin.
	 * - Linked pin => <SourceNodeTitle>.<SourcePinName> (mirrors legacy BlueprintExporter behavior)
	 */
	static FString DescribePinResolvedValue(UEdGraphPin* Pin);

	/** Export a UE-style copy/paste snippet for a given graph into the Snippets directory. */
	static void ExportGraphSnippet(UBlueprint* Blueprint, UEdGraph* Graph, const FString& SnippetDirectory);

	/** High-level export entrypoint driven by UI settings. */
	static void ExportAllForFormat(const FString& OutputRoot, const FString& RootPath, EBEPExportOutputFormat OutputFormat, const TArray<FString>& ExcludedClassPatterns, int32 MaxAssetsPerRun);

	/** Returns true if the asset should be skipped based on defaults and user patterns. */
	static bool ShouldSkipAsset(const FAssetData& AssetData, const TArray<FString>& UserExcludedClassPatterns);

private:
	/** Default excluded class names (textures, meshes, raw anims, etc.). */
	static const TSet<FName>& GetDefaultExcludedClassNames();

	/** Split comma/newline separated patterns into an array. */
	static void ParseExcludedPatterns(const FString& PatternsString, TArray<FString>& OutPatterns);

	/** Collect all relevant graphs from a Blueprint (ubergraph, functions, macros, delegate signatures). */
	static void GetAllGraphsForBlueprint(UBlueprint* Blueprint, TArray<UEdGraph*>& OutGraphs);
};
