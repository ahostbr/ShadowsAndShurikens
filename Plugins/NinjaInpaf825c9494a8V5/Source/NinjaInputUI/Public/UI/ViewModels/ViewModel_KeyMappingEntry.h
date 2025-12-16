// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputBaseViewModel.h"
#include "Styling/SlateBrush.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModel_KeyMappingEntry.generated.h"

class UNinjaInputPlayerMappableKeySettings;

/**
 * Represents a specific key mapping entry.
 */
UCLASS(DisplayName = "Key Mapping Entry")
class NINJAINPUTUI_API UViewModel_KeyMappingEntry : public UNinjaInputBaseViewModel
{
	
	GENERATED_BODY()

public:

	// -- Begin InputViewModel implementation
	virtual void ShutdownViewModel_Implementation() override;
	// -- End InputViewModel implementation
	
	void SetPlayerController(APlayerController* OwningPlayer);
	void SetMapping(const FPlayerKeyMapping& Mapping, const UNinjaInputPlayerMappableKeySettings* Settings);
	
	void SetMappingName(const FName& NewValue);
	void SetDisplayName(const FText& NewValue);
	void SetDisplayCategory(const FText& NewValue);
	void SetTooltip(const FText& NewValue);
	void SetCurrentKey(const FKey& NewValue);
	void SetDefaultKey(const FKey& NewValue);
	void SetIconBrush(const FSlateBrush& NewValue);
	void SetIsCustomized(const bool bNewValue);

	/**
	 * Rebinds this entry, using the entire set of key arguments. 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Key Mapping Entry")
	void RebindKey(const FMapPlayerKeyArgs& NewKeyArgs) const;

	/**
	 * Rebinds this entry to the provided key.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Key Mapping Entry")
	void RebindKeyToFirstSlot(const FKey& Key) const;

	/**
	 * Resets this entry to its default input.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintCosmetic, Category = "Ninja Input|ViewModel|Key Mapping Entry")
	void ResetKeyBinding() const;
	
protected:

	/** Mapping name that uniquely identifies this binding. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FName MappingName = NAME_None;

	/** Name configured for the UI, in the binding settings. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FText DisplayName = FText();

	/** Category configured for the UI, in the binding settings. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FText DisplayCategory = FText();

	/** Tooltip added to this mapping. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FText Tooltip = FText();
	
	/** Current key assigned to this binding. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FKey CurrentKey;

	/** Default key assigned to this binding. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FKey DefaultKey;

	/** Icon representing the key, following the Common UI platform key settings. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter)
	FSlateBrush IconBrush;
	
	/** Informs if this binding was customized by the player or not. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Entry View Model", Setter = SetIsCustomized)
	bool bIsCustomized = false;

	/**
	 * Updates the Icon Brush setting, using the Common UI platform icons set in the project.
	 * The value will be retrieved, based on the current value set for "CurrentKey".
	 */
	virtual void UpdateIconBrushFromCurrentKey();

private:

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	
};
