// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputBaseViewModel.h"
#include "ViewModel_KeyMappings.h"
#include "ViewModel_KeyMappingCategory.generated.h"

class UViewModel_KeyMappingEntry;

/**
 * Represents one Key Binding category, and includes its mappings.
 */
UCLASS(DisplayName = "Key Mapping Category")
class NINJAINPUTUI_API UViewModel_KeyMappingCategory : public UNinjaInputBaseViewModel
{
	
	GENERATED_BODY()

public:

	// -- Begin InputViewModel implementation
	virtual void ShutdownViewModel_Implementation() override;
	// -- End InputViewModel implementation

	void SetPlayerController(APlayerController* OwningPlayer);
	void SetDisplayCategory(const FText& NewValue);
	void SetMappings(const TArray<FPlayerKeyMapping>& Mappings);

	/**
	 * Provides the bindings contained in this category.
	 * 
	 * The Binding array is sorted alphabetically, based on the display name. But you can
	 * order it differently by feeding the result into another "Sorting ViewModel" or an
	 * equivalent helper function.
	 */
	UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "Ninja Input|ViewModel|Key Mapping Category", meta = (ReturnDisplayName = "Bindings"))
	TArray<UViewModel_KeyMappingEntry*> GetBindings() const;	

	/**
	 * Provides all bindings in this category, related to the Keyboard and Mouse.
	 */
	UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "Ninja Input|ViewModel|Key Mapping Category", meta = (ReturnDisplayName = "Bindings"))
	TArray<UViewModel_KeyMappingEntry*> GetKeyboardAndMouseBindings() const;	

	/**
	 * Provides all bindings in this category, related to the Gamepad.
	 */
	UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "Ninja Input|ViewModel|Key Mapping Category", meta = (ReturnDisplayName = "Bindings"))
	TArray<UViewModel_KeyMappingEntry*> GetGamepadBindings() const;
	
protected:

	/** Name set for this category. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Key Mapping Category View Model", Setter)
	FText DisplayCategory;

private:

	/** The player controller that will be sent forward, for input remappings. */
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	
	TArray<FPlayerKeyMapping> OriginalMappings;

	/** Stores the complete list of bindings, regardless of their filters. */
	UPROPERTY()
	TArray<TObjectPtr<UViewModel_KeyMappingEntry>> Bindings;

	/** An organized list of bindings related to the Keyboard and Mouse. */
	UPROPERTY()
	TArray<TObjectPtr<UViewModel_KeyMappingEntry>> KeyboardAndMouseBindings;	

	/** An organized list of bindings related to the Gamepad. */
	UPROPERTY()
	TArray<TObjectPtr<UViewModel_KeyMappingEntry>> GamepadBindings;
	
};

