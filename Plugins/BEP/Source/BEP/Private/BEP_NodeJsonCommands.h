#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FBEPNodeJsonCommands : public TCommands<FBEPNodeJsonCommands>
{
public:
    FBEPNodeJsonCommands()
        : TCommands<FBEPNodeJsonCommands>(TEXT("BEPNodeJson"), NSLOCTEXT("BEP", "BEPNodeJson", "BEP Node JSON"), NAME_None, FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenPanel;
    TSharedPtr<FUICommandInfo> ExportSelection;
    TSharedPtr<FUICommandInfo> CopySelection;
    TSharedPtr<FUICommandInfo> ExportComments;
    TSharedPtr<FUICommandInfo> ImportComments;
    TSharedPtr<FUICommandInfo> WriteGoldenSamples;
    TSharedPtr<FUICommandInfo> SelfCheck;
};
