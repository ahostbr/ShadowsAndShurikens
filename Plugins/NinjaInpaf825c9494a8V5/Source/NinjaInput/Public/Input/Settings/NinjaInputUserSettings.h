// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputDelegates.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "NinjaInputUserSettings.generated.h"

/** 
 * Stores additional settings for any input-related functionality.
 */
UCLASS()
class NINJAINPUT_API UNinjaInputUserSettings : public UEnhancedInputUserSettings
{
	
	GENERATED_BODY()

public:

	/** Notifies when the asynchronous process of saving settings has started. */
	UPROPERTY(BlueprintAssignable)
	FInputSaveSettingsEventSignature OnSaveSettingsStarted;

	/** Notifies when the asynchronous process of saving settings has successfully ended. */	
	UPROPERTY(BlueprintAssignable)
	FInputSaveSettingsEventSignature OnSaveSettingsCompleted;

	/** Notifies when the asynchronous process of saving settings has failed. */
	UPROPERTY(BlueprintAssignable)
	FInputSaveSettingsEventSignature OnSaveSettingsFailed;
	
	// -- Begin EnhancedInputUserSettings implementation
	virtual void AsyncSaveSettings() override;
	virtual void OnAsyncSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess) override;
	// -- End EnhancedInputUserSettings implementation

	/**
	 * Informs if the vertical axis should be inverted.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|User Settings")
	bool ShouldInvertVerticalAxis() const { return bInvertVerticalAxis; }

	/**
	 * Informs if the horizontal axis should be inverted.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|User Settings")
	bool ShouldInvertHorizontalAxis() const { return bInvertHorizontalAxis; }

	/**
	 * Provides the multiplier for the mouse sensitivity on the X axis.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|User Settings")
	float GetMouseSensitivityX() const { return MouseSensitivityX; }

	/**
	 * Provides the multiplier for the mouse sensitivity on the Y axis.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|User Settings")
	float GetMouseSensitivityY() const { return MouseSensitivityY; }

	/**
	 * Provides the multiplier for the gamepad sensitivity.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|User Settings")
	float GetGamepadSensitivity() const { return GamepadSensitivity; }
	
	/**
	 * Defines if the vertical axis should be inverted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|User Settings")
	void SetInvertVerticalAxis(bool bNewValue);
	
	/**
	 * Defines if the horizontal axis should be inverted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|User Settings")
	void SetInvertHorizontalAxis(bool bNewValue);

	/**
	 * Defines the mouse sensitivity on the X Axis.
	 */	
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|User Settings")
	void SetMouseSensitivityX(const float NewValue);

	/**
	 * Defines the mouse sensitivity on the Y Axis.
	 */	
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|User Settings")
	void SetMouseSensitivityY(const float NewValue);

	/**
	 * Defines the gamepad sensitivity.
	 */	
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|User Settings")
	void SetGamepadSensitivity(const float NewValue);
	
protected:

	/** Inverts the vertical axis for a given 2D input. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|Ninja Input Settings")
	bool bInvertVerticalAxis = false;

	/** Inverts the horizontal axis for a given 2D input. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|Ninja Input Settings")
	bool bInvertHorizontalAxis = false;

	/** Modifies the mouse sensitivity on the X axis. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|Ninja Input Settings", meta = (ClampMin = 0.f, ClampMax = 10.f, UIMin = 0.f, UIMax = 10.f))
	float MouseSensitivityX = 1.f;

	/** Modifies the mouse sensitivity on the Y axis. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|Ninja Input Settings", meta = (ClampMin = 0.f, ClampMax = 10.f, UIMin = 0.f, UIMax = 10.f))
	float MouseSensitivityY = 1.f;

	/** Scales the gamepad sensitivity on both axis. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|Ninja Input Settings", meta = (ClampMin = 0.f, ClampMax = 10.f, UIMin = 0.f, UIMax = 10.f))
	float GamepadSensitivity = 1.f;

};
