// Ninja Bear Studio Inc., all rights reserved.
#include "UI/Resolvers/NinjaInputContextResolver.h"

#include "UI/ViewModels/NinjaInputBaseViewModel.h"

UObject* UNinjaInputContextResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UNinjaInputBaseViewModel* ViewModel = NewObject<UNinjaInputBaseViewModel>(GetOuter(), ExpectedType);
	check(IsValid(ViewModel));

	ViewModel->InitializeViewModel(UserWidget);
	return ViewModel;
}

void UNinjaInputContextResolver::DestroyInstance(const UObject* ViewModel, const UMVVMView* View) const
{
	const UNinjaInputBaseViewModel* CustomViewModel = Cast<UNinjaInputBaseViewModel>(ViewModel);
	if (IsValid(CustomViewModel))
	{
		UNinjaInputBaseViewModel* MutableViewModel = const_cast<UNinjaInputBaseViewModel*>(CustomViewModel);
		MutableViewModel->ShutdownViewModel();
	}
}

#if WITH_EDITOR
bool UNinjaInputContextResolver::DoesSupportViewModelClass(const UClass* Class) const
{
	if (Class->IsChildOf(UNinjaInputBaseViewModel::StaticClass()))
	{
		return true;
	}

	return Super::DoesSupportViewModelClass(Class);
}
#endif