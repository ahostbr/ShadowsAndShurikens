// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_BaseAsyncTask.h"

#include "UGC_EditorNodes.generated.h"

UCLASS(ClassGroup = "UGC Camera Animations")
class UUGC_PlayCameraAnimation : public UK2Node_BaseAsyncTask
{
    GENERATED_UCLASS_BODY()

    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetMenuCategory() const override;
};

UCLASS(ClassGroup = "UGC Camera Animations")
class UUGC_PlayCameraAnimationWithModifier : public UK2Node_BaseAsyncTask
{
    GENERATED_UCLASS_BODY()

    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetMenuCategory() const override;
};