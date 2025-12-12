
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LightProbeDebugWidget.generated.h"

class UTextureRenderTarget2D;
class SImage;

UCLASS()
class LIGHTPROBEPLUGIN_API ULightProbeDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    UTextureRenderTarget2D* RenderTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    FVector2D ImageSize = FVector2D(256.f, 256.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    FVector2D ScreenOffset = FVector2D(16.f, 16.f);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;

    void UpdateBrush();

    TSharedPtr<SImage> MyImage;
    FSlateBrush Brush;
};
