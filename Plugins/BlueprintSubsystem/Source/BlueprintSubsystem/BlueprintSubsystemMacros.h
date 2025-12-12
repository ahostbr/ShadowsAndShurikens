// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "Engine/BlueprintGeneratedClass.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"

#define IMPLEMENTED_IN_BP_LAMBDA() auto ImplementedInBlueprint = [](const UFunction* Func) -> bool \
{ \
	return Func && ensure(Func->GetOuter()) && Func->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()); \
}

#define CHECK_IN_BP(Class, BaseName) \
{ \
	static const FName FuncName = GET_FUNCTION_NAME_CHECKED(Class, K2_##BaseName); \
	const UFunction* Func = FindFunction(FuncName); \
	bHasBlueprint##BaseName = ImplementedInBlueprint(Func); \
}
