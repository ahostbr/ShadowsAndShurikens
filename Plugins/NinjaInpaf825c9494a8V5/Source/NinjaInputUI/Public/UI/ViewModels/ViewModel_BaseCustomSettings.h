// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputBaseViewModel.h"
#include "ViewModel_BaseCustomSettings.generated.h"

class UNinjaInputUserSettings;

/**
 * Exposes operations that can modify the custom settings class. 
 */
UCLASS(Abstract, NotBlueprintable)
class NINJAINPUTUI_API UViewModel_BaseCustomSettings : public UNinjaInputBaseViewModel
{
	
	GENERATED_BODY()

public:

	// -- Begin InputViewModel implementation
	virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;
	virtual void ShutdownViewModel_Implementation() override;
	// -- End InputViewModel implementation
	
	void SetIsSavingSettings(bool bNewValue);

	/**
	 * Provides the User Settings instance.
	 */
	UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Custom Settings")
	UNinjaInputUserSettings* GetUserSettings() const;
	
	/**
	 * Saves the current user settings values, requesting an asynchronous operation.
	 * Will not start a save operation if another is in progress. 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Custom Settings")
	void SaveInputSettings();
	
protected:

	/** Player controller that owns the custom settings class. */
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	
	/** Informs when the background asynchronous saving process is happening. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Settings", Setter = SetIsSavingSettings)
	bool bIsSavingSettings;
	
	/**
	 * Triggered when we start saving our user settings.
	 */
	UFUNCTION()
	virtual void HandleSaveSettingsStarted();

	/**
	 * Triggered when we finished (completed/failed) our saving process.
	 * This also removes any dependencies from the settings class itself.
	 */	
	UFUNCTION()
	virtual void OnSaveSettingsFinished();

	/**
	 * Clears bindings and resets the internal pointer to the user settings.
	 */
	void ResetUserSettings();
	
private:
	
	/**
	 * Stores the user settings when we are performing a save operation.
	 *
	 * This value is only present during the save operation, while we are listening to its delegates.
	 * When the save operation finishes, we unbind delegates and clear this reference.
	 */
	UPROPERTY()
	TObjectPtr<UNinjaInputUserSettings> UserSettings;	
	
};
