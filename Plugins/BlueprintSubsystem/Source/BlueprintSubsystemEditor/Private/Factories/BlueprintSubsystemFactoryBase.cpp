// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "BlueprintSubsystemFactoryBase.h"
#include "KismetCompilerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Subsystems/BlueprintGameInstanceSubsystem.h"
#include "Subsystems/BlueprintWorldSubsystem.h"

UBlueprintSubsystemFactoryBase::UBlueprintSubsystemFactoryBase()
	: ParentClass(nullptr)
{
	bCreateNew = bEditAfterNew = true;
	SupportedClass = UBlueprint::StaticClass();
}

UObject* UBlueprintSubsystemFactoryBase::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	check(InClass->IsChildOf(UBlueprint::StaticClass()));

	if ((ParentClass == nullptr) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != nullptr) ? FText::FromString( ParentClass->GetName() ) : INVTEXT("null"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("BlueprintSubsystem", "CannotCreateBlueprintFromClass", "Cannot create a blueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}

	UClass* BlueprintClass = nullptr;
	UClass* BlueprintGeneratedClass = nullptr;

	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetBlueprintTypesForClass(ParentClass, BlueprintClass, BlueprintGeneratedClass);

	return FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, InName, BPTYPE_Normal, BlueprintClass, BlueprintGeneratedClass, CallingContext);
}

UBlueprintGameInstanceSubsystemFactory::UBlueprintGameInstanceSubsystemFactory()
{
	ParentClass = UBlueprintGameInstanceSubsystem::StaticClass();
}

UBlueprintWorldSubsystemFactory::UBlueprintWorldSubsystemFactory()
{
	ParentClass = UBlueprintWorldSubsystem::StaticClass();
}
