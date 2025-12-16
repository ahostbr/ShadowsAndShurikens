#include "BEP_NodeJsonExportSettings.h"

#include "BEP_NodeJsonExport.h"

void UBEP_NodeJsonExportSettings::ApplyPresetToSettings(EBEP_NodeJsonPreset Preset)
{
	FBEP_NodeJsonExportOptions Opt;
	BEP_NodeJson::ApplyPreset(Opt, Preset);

	DefaultPreset = Preset;
	ExecDepth = Opt.ExecDepth;
	DataDepth = Opt.DataDepth;
	bPrettyPrint = Opt.bPrettyPrint;
	bCompactKeys = Opt.bCompactKeys;
	bIncludeLegend = Opt.bIncludeLegend;
	bIncludeClassDict = Opt.bIncludeClassDict;
	bIncludePosition = Opt.bIncludePosition;
	bIncludePinDecls = Opt.bIncludePinDecls;
	bIncludeFullPins = Opt.bIncludeFullPins;
	bIncludeUnconnectedPins = Opt.bIncludeUnconnectedPins;
	bIncludeExecEdges = Opt.bIncludeExecEdges;
	bIncludeDataEdges = Opt.bIncludeDataEdges;
	bIncludeNodeTitle = Opt.bIncludeNodeTitle;
	bIncludeNodeComment = Opt.bIncludeNodeComment;
	MaxNodesHardCap = Opt.MaxNodesHardCap;
	MaxEdgesHardCap = Opt.MaxEdgesHardCap;
	bWarnOnCapHit = Opt.bWarnOnCapHit;
}

void UBEP_NodeJsonExportSettings::BuildExportOptions(FBEP_NodeJsonExportOptions& OutOptions) const
{
	BEP_NodeJson::ApplyPreset(OutOptions, DefaultPreset);
	OutOptions.ExecDepth = ExecDepth;
	OutOptions.DataDepth = DataDepth;
	OutOptions.bPrettyPrint = bPrettyPrint;
	OutOptions.bCompactKeys = bCompactKeys;
	OutOptions.bIncludeLegend = bIncludeLegend;
	OutOptions.bIncludeClassDict = bIncludeClassDict;
	OutOptions.bIncludePosition = bIncludePosition;
	OutOptions.bIncludePinDecls = bIncludePinDecls;
	OutOptions.bIncludeFullPins = bIncludeFullPins;
	OutOptions.bIncludeUnconnectedPins = bIncludeUnconnectedPins;
	OutOptions.bIncludeExecEdges = bIncludeExecEdges;
	OutOptions.bIncludeDataEdges = bIncludeDataEdges;
	OutOptions.bIncludeNodeTitle = bIncludeNodeTitle;
	OutOptions.bIncludeNodeComment = bIncludeNodeComment;
	OutOptions.MaxNodesHardCap = MaxNodesHardCap;
	OutOptions.MaxEdgesHardCap = MaxEdgesHardCap;
	OutOptions.bWarnOnCapHit = bWarnOnCapHit;
}
