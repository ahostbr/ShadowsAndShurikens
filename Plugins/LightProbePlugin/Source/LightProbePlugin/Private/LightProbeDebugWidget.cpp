
#include "LightProbeDebugWidget.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateBrush.h"

TSharedRef<SWidget> ULightProbeDebugWidget::RebuildWidget()
{
    TSharedRef<SImage> ImageWidget = SNew(SImage);
    MyImage = ImageWidget;

    UpdateBrush();

    return SNew(SOverlay)
    + SOverlay::Slot()
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Top)
    .Padding(FMargin(ScreenOffset.X, ScreenOffset.Y, 0.f, 0.f))
    [
        SNew(SBox)
        .WidthOverride(ImageSize.X)
        .HeightOverride(ImageSize.Y)
        [
            ImageWidget
        ]
    ];
}

void ULightProbeDebugWidget::UpdateBrush()
{
    if (!MyImage.IsValid())
    {
        return;
    }

    if (RenderTarget)
    {
        Brush.SetResourceObject(RenderTarget);
        Brush.ImageSize = ImageSize;
        MyImage->SetImage(&Brush);
    }
    else
    {
        MyImage->SetImage(nullptr);
    }
}

void ULightProbeDebugWidget::NativeDestruct()
{
    MyImage.Reset();
    Super::NativeDestruct();
}
