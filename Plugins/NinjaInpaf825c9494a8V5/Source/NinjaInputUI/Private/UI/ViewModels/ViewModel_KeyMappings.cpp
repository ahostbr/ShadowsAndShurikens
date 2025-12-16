// Ninja Bear Studio Inc., all rights reserved.
#include "UI/ViewModels/ViewModel_KeyMappings.h"

#include "NinjaInputFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/NinjaInputManagerComponent.h"
#include "Input/Settings/NinjaInputUserSettings.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UI/ViewModels/ViewModel_KeyMappingCategory.h"

TArray<UViewModel_KeyMappingCategory*> UViewModel_KeyMappings::GetCategories() const
{
	return Categories;
}

void UViewModel_KeyMappings::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
	Super::InitializeViewModel_Implementation(UserWidget);
	if (!IsValid(PlayerController))
	{
		return;
	}

	const UEnhancedPlayerMappableKeyProfile* Profile = GetProfile(PlayerController);
	if (!IsValid(Profile))
	{
		return;
	}

	const TMap<FName, FKeyMappingRow>& Rows = Profile->GetPlayerMappingRows();
	ProcessInputRows(UserWidget, Rows);		
}

const UEnhancedPlayerMappableKeyProfile* UViewModel_KeyMappings::GetProfile(const APlayerController* OwningPlayer)
{
	const UNinjaInputManagerComponent* InputManager = UNinjaInputFunctionLibrary::GetInputManagerComponent(OwningPlayer);
	if (!IsValid(InputManager))
	{
		return nullptr;
	}

	UNinjaInputUserSettings* Settings = InputManager->GetInputUserSettings();
	if (!IsValid(Settings))
	{
		return nullptr;
	}
	
#if (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6))
	return Settings->GetActiveKeyProfile();
#else
	return Settings->GetCurrentKeyProfile();
#endif
}

void UViewModel_KeyMappings::ProcessInputRows(const UUserWidget* UserWidget, const TMap<FName, FKeyMappingRow>& Rows)
{
	// Group by a lightweight and stable key (FName). We'll store the display text in the value.
	struct FCategoryBucket
	{
		FText DisplayCategory;
		TArray<FPlayerKeyMapping> Mappings;
	};

	TMap<FName, FCategoryBucket> CategoryMap;
	CategoryMap.Reserve(Rows.Num());
	
	// In the first pass, we need to recreate the structure, so it's organized by categories.
	for (const TPair<FName, FKeyMappingRow>& Entry : Rows)
	{
		for (const FPlayerKeyMapping& Mapping : Entry.Value.Mappings)
		{
			const FText DisplayCategory = Mapping.GetDisplayCategory();
			const FName CategoryId(*DisplayCategory.ToString());

			FCategoryBucket& Bucket = CategoryMap.FindOrAdd(CategoryId);
			if (Bucket.DisplayCategory.IsEmpty())
			{
				Bucket.DisplayCategory = DisplayCategory;
			}
			Bucket.Mappings.Add(Mapping);
		}
	}

	// Build a sorted view for the categories.
	TArray<FCategoryBucket> Ordered;
	Ordered.Reserve(CategoryMap.Num());
	for (const TPair<FName, FCategoryBucket>& Pair : CategoryMap)
	{
		Ordered.Add(Pair.Value);
	}

	Ordered.Sort([](const FCategoryBucket& A, const FCategoryBucket& B)
	{
		return A.DisplayCategory.CompareTo(B.DisplayCategory) < 0;
	});	

	ResetCategories();
	Categories.Reserve(Ordered.Num());
	
	// Now we are ready to create the ViewModels for all categories/mappings.
	for (FCategoryBucket& Bucket : Ordered)
	{
		UViewModel_KeyMappingCategory* Category = NewObject<UViewModel_KeyMappingCategory>(UserWidget->GetOuter());
		if (IsValid(Category))
		{
			Category->SetDisplayCategory(Bucket.DisplayCategory);
			Category->SetMappings(MoveTemp(Bucket.Mappings));
			Categories.Add(Category);
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCategories);
}

void UViewModel_KeyMappings::ResetCategories()
{
	for (UViewModel_KeyMappingCategory* Category : Categories)
	{
		Category->ShutdownViewModel();		
	}

	Categories.Empty();
}

void UViewModel_KeyMappings::ShutdownViewModel_Implementation()
{
	Super::ShutdownViewModel_Implementation();
	ResetCategories();
}
