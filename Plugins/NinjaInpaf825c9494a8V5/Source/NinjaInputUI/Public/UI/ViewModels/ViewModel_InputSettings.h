// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "ViewModel_BaseCustomSettings.h"
#include "ViewModel_InputSettings.generated.h"

/**
 * Exposes input settings from the custom input settings class.
 */
UCLASS(DisplayName = "Input Settings")
class NINJAINPUTUI_API UViewModel_InputSettings : public UViewModel_BaseCustomSettings
{
	
	GENERATED_BODY()

public:

	// -- Begin InputViewModel implementation
	virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;
	// -- End InputViewModel implementation
	
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Input Settings")
	void SetInvertVerticalAxis(const bool bNewValue);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Input Settings")
	void SetInvertHorizontalAxis(const bool bNewValue);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Input Settings")
	void SetMouseSensitivityX(const float NewValue);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Input Settings")
	void SetMouseSensitivityY(const float NewValue);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Input Settings")
	void SetGamepadSensitivity(const float NewValue);

protected:

	/** Inverts the vertical axis for a given 2D input. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category="Ninja Input Settings", Setter = SetInvertVerticalAxis)
	bool bInvertVerticalAxis = false;

	/** Inverts the horizontal axis for a given 2D input. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category="Ninja Input Settings", Setter = SetInvertHorizontalAxis)
	bool bInvertHorizontalAxis = false;

	/** Modifies the mouse sensitivity on the X axis. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category="Ninja Input Settings", Setter)
	float MouseSensitivityX = 1.f;

	/** Modifies the mouse sensitivity on the Y axis. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category="Ninja Input Settings", Setter)
	float MouseSensitivityY = 1.f;

	/** Scales the gamepad sensitivity on both axis. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category="Ninja Input Settings", Setter)
	float GamepadSensitivity = 1.f;	

};
