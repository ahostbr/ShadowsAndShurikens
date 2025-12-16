// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "NinjaInputBaseViewModel.generated.h"

/**
 * Internal core class for all view models related to the Input System. 
 */
UCLASS(Abstract, NotBlueprintable)
class NINJAINPUTUI_API UNinjaInputBaseViewModel : public UMVVMViewModelBase
{
	
	GENERATED_BODY()

public:

	/**
	 * Initializes this view model for a given widget.
	 *
	 * @param UserWidget
	 *		The User Widget requesting this View Model.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel")	
	void InitializeViewModel(const UUserWidget* UserWidget);

	/**
	 * An opportunity for the view model to perform any clean-up tasks.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "Ninja Input|ViewModel")
	void ShutdownViewModel();
	
};
