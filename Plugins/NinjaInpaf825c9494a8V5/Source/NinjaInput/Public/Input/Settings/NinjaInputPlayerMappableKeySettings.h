// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "PlayerMappableKeySettings.h"
#include "NinjaInputPlayerMappableKeySettings.generated.h"

/**
 * Additional data available for each mappable key in the project.
 */
UCLASS()
class NINJAINPUT_API UNinjaInputPlayerMappableKeySettings : public UPlayerMappableKeySettings
{
	
	GENERATED_BODY()

public:

	/**
	 * Provides the input mode used to filter this in the UI.
	 */
	const FGameplayTag& GetInputFilter() const;
	
	/**
	 * Returns the tooltip displayed in the user interface.
	 */
	const FText& GetTooltipText() const;

protected:
	
	/** The category used to filter this input, so ViewModels can organize this correctly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayAfter = Name, Categories = "Input.Mode"))
	FGameplayTag InputFilter = FGameplayTag::EmptyTag;

	/** The tooltip that should be associated with this action when displayed on the settings screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayAfter = DisplayCategory))
	FText Tooltip = FText::GetEmpty();
	
};
