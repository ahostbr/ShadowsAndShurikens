#include "BEP_NodeJsonCommands.h"

#define LOCTEXT_NAMESPACE "BEPNodeJsonCommands"

void FBEPNodeJsonCommands::RegisterCommands()
{
    UI_COMMAND(OpenPanel, "BEP: Node JSON Panel…", "Open the BEP Node JSON dockable panel.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(ExportSelection, "BEP: Export Node JSON (Selection)", "Export selected Blueprint graph nodes to JSON.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CopySelection, "BEP: Copy Node JSON (Selection)", "Copy selected Blueprint graph nodes as JSON to clipboard.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(ExportComments, "BEP: Export Comment JSON (Selection)", "Export selected nodes as Comment JSON.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(ImportComments, "BEP: Import Comments CSV…", "Import comments from a CSV and spawn comment boxes.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(WriteGoldenSamples, "BEP: Write Golden Samples", "Write fixed golden sample outputs for regression diffs.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(SelfCheck, "BEP: Node JSON (Self Check)", "Run a non-writing environment self-check for Node JSON.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
