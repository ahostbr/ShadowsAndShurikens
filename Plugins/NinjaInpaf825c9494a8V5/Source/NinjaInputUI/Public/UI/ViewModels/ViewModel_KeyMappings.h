// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "ViewModel_BaseCustomSettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModel_KeyMappings.generated.h"

class UEnhancedPlayerMappableKeyProfile;
class UNinjaInputUserSettings;
class UViewModel_KeyMappingCategory;

/**
 * ViewModel that exposes current key mappings from the active profile, organized by category.
 */
UCLASS(DisplayName = "Key Mappings")
class NINJAINPUTUI_API UViewModel_KeyMappings : public UViewModel_BaseCustomSettings
{
	
	GENERATED_BODY()

public:

	// -- Begin InputViewModel implementation
	virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;
	virtual void ShutdownViewModel_Implementation() override;
	// -- End InputViewModel implementation
	
	/**
	 * Provides the categories for Key Mappings. Each category has its own mappings.
	 * 
	 * The Category array is sorted alphabetically, based on the category's name. But you can
	 * order it differently by feeding the result into another "Sorting ViewModel" or an
	 * equivalent helper function.
	 */
	UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "Ninja Input|ViewModel|Key Mappings", meta = (ReturnDisplayName = "Categories"))
	TArray<UViewModel_KeyMappingCategory*> GetCategories() const;
	
protected:
	
	/**
	 * Provides the Key Profile, respecting engine APIs.
	 */
	static const UEnhancedPlayerMappableKeyProfile* GetProfile(const APlayerController* OwningPlayer);

	/**
	 * Organizes the rows into their own category groups and broadcast the update.
	 */
	void ProcessInputRows(const UUserWidget* UserWidget, const TMap<FName, FKeyMappingRow>& Rows);

	/**
	 * Resets current category ViewModels, allowing them to also shut down.
	 */
	void ResetCategories();

	/**
	 * Unbinds and resets from the user settings.
	 */
	void ResetUserSettings();
	
private:

	UPROPERTY()
	TArray<TObjectPtr<UViewModel_KeyMappingCategory>> Categories;
	
};
