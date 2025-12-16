// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "NinjaInputContextResolver.generated.h"

/**
 * Provides view model classes for the input system.
 */
UCLASS(DisplayName = "Input Resolver")
class NINJAINPUTUI_API UNinjaInputContextResolver : public UMVVMViewModelContextResolver
{
	
	GENERATED_BODY()

public:

	// -- Begin Model Context Resolver implementation
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(const UObject* ViewModel, const UMVVMView* View) const override;
	// -- End Model Context Resolver implementation
	
#if WITH_EDITOR
	
	// -- Begin Editor implementation
	virtual bool DoesSupportViewModelClass(const UClass* Class) const override;
	// -- End Editor implementation
	
#endif
	
};
