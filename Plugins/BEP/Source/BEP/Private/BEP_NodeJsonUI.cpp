#include "BEP_NodeJsonUI.h"

#include "BEP_NodeJsonExport.h"
#include "BEP_NodeJsonExportSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

FBEP_NodeJsonExportOptions BEPNodeJsonUI::BuildOptionsFromSettings()
{
    FBEP_NodeJsonExportOptions Opt;
    const UBEP_NodeJsonExportSettings* Settings = GetDefault<UBEP_NodeJsonExportSettings>();
    if (!Settings)
    {
        BEP_NodeJson::ApplyPreset(Opt, EBEP_NodeJsonPreset::SuperCompact);
        return Opt;
    }

    Settings->BuildExportOptions(Opt);
    return Opt;
}

void BEPNodeJsonUI::ShowNodeJsonToast(const FString& Message, bool bSuccess)
{
    FNotificationInfo Info(FText::FromString(Message));
    Info.bFireAndForget = true;
    Info.FadeInDuration = 0.05f;
    Info.FadeOutDuration = 0.2f;
    Info.ExpireDuration = 2.5f;

    TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
    if (Item.IsValid())
    {
        Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
    }
}
