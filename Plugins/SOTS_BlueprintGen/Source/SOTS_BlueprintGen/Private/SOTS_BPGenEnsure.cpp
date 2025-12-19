#include "SOTS_BPGenEnsure.h"

#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenEditGuard.h"
#include "SOTS_BPGenGraphResolver.h"
#include "SOTS_BlueprintGen.h"

#include "UObject/Class.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Blueprint/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ContentWidget.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"

namespace
{
	static const FString ErrorInvalidInput = TEXT("ERR_INVALID_INPUT");
	static const FString ErrorTargetNotFound = TEXT("ERR_TARGET_NOT_FOUND");
	static const FString ErrorTargetCreateFailed = TEXT("ERR_TARGET_CREATE_FAILED");
	static const FString ErrorTypeInvalid = TEXT("ERR_TYPE_INVALID");
	static const FString ErrorWidgetClassInvalid = TEXT("ERR_WIDGET_CLASS_INVALID");
	static const FString ErrorWidgetNotFound = TEXT("ERR_WIDGET_NOT_FOUND");
	static const FString ErrorWidgetParentInvalid = TEXT("ERR_WIDGET_PARENT_INVALID");
	static const FString ErrorWidgetProperty = TEXT("ERR_WIDGET_PROPERTY");

	static bool MakePinTypeFromSpec(const FSOTS_BPGenPin& InPin, FEdGraphPinType& OutType)
	{
		OutType.ResetToDefaults();
		OutType.PinCategory = InPin.Category;
		OutType.PinSubCategory = InPin.SubCategory;
		OutType.PinSubCategoryObject = nullptr;

		if (!InPin.SubObjectPath.IsEmpty())
		{
			if (UObject* LoadedObj = LoadObject<UObject>(nullptr, *InPin.SubObjectPath))
			{
				OutType.PinSubCategoryObject = LoadedObj;
			}
		}

		switch (InPin.ContainerType)
		{
		case ESOTS_BPGenContainerType::Array:
			OutType.ContainerType = EPinContainerType::Array;
			break;
		case ESOTS_BPGenContainerType::Set:
			OutType.ContainerType = EPinContainerType::Set;
			break;
		case ESOTS_BPGenContainerType::Map:
			OutType.ContainerType = EPinContainerType::Map;
			break;
		default:
			OutType.ContainerType = EPinContainerType::None;
			break;
		}

		return !OutType.PinCategory.IsNone();
	}

	static bool SaveBlueprint(UBlueprint* Blueprint, FString& OutError)
	{
		if (!Blueprint)
		{
			OutError = TEXT("SaveBlueprint: Blueprint was null.");
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		if (!Package)
		{
			OutError = TEXT("SaveBlueprint: Package was null.");
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bWarnOfLongFilename = false;

		if (!UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs))
		{
			OutError = TEXT("SaveBlueprint: SavePackage returned false.");
			return false;
		}

		return true;
	}

	static bool TryLoadWidgetBlueprint(const FString& BlueprintAssetPath, UWidgetBlueprint*& OutWidgetBlueprint, FString& OutError, FString& OutErrorCode)
	{
		OutWidgetBlueprint = nullptr;
		if (BlueprintAssetPath.IsEmpty())
		{
			OutErrorCode = ErrorInvalidInput;
			OutError = TEXT("BlueprintAssetPath was empty.");
			return false;
		}

		UBlueprint* Loaded = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintAssetPath));
		if (!Loaded)
		{
			OutErrorCode = ErrorTargetNotFound;
			OutError = FString::Printf(TEXT("Failed to load Blueprint '%s'."), *BlueprintAssetPath);
			return false;
		}

		OutWidgetBlueprint = Cast<UWidgetBlueprint>(Loaded);
		if (!OutWidgetBlueprint)
		{
			OutErrorCode = ErrorTypeInvalid;
			OutError = TEXT("Target blueprint is not a WidgetBlueprint.");
			return false;
		}

		return true;
	}

	static UClass* ResolveWidgetClass(const FString& ClassPath)
	{
		UClass* WidgetClass = FindObject<UClass>(nullptr, *ClassPath);
		if (!WidgetClass)
		{
			WidgetClass = LoadObject<UClass>(nullptr, *ClassPath);
		}
		return WidgetClass;
	}

	static bool ResolvePropertyPath(UObject* Root, const TArray<FString>& Segments, void*& OutContainerPtr, FProperty*& OutProperty, FString& OutError)
	{
		OutContainerPtr = nullptr;
		OutProperty = nullptr;

		if (!Root || Segments.Num() == 0)
		{
			OutError = TEXT("Invalid property path.");
			return false;
		}

		void* CurrentPtr = Root;
		UStruct* CurrentStruct = Root->GetClass();

		for (int32 Index = 0; Index < Segments.Num(); ++Index)
		{
			const FName SegmentName(*Segments[Index]);
			FProperty* Property = CurrentStruct ? CurrentStruct->FindPropertyByName(SegmentName) : nullptr;
			if (!Property)
			{
				OutError = FString::Printf(TEXT("Property '%s' not found."), *Segments[Index]);
				return false;
			}

			const bool bIsLeaf = (Index == Segments.Num() - 1);
			if (bIsLeaf)
			{
				OutContainerPtr = CurrentPtr;
				OutProperty = Property;
				return true;
			}

			if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				CurrentPtr = StructProp->ContainerPtrToValuePtr<void>(CurrentPtr);
				CurrentStruct = StructProp->Struct;
				continue;
			}

			if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
			{
				UObject* const* ObjectPtr = ObjectProp->ContainerPtrToValuePtr<UObject*>(CurrentPtr);
				if (!ObjectPtr || !(*ObjectPtr))
				{
					OutError = FString::Printf(TEXT("Object property '%s' was null."), *Segments[Index]);
					return false;
				}
				CurrentPtr = *ObjectPtr;
				CurrentStruct = (*ObjectPtr)->GetClass();
				continue;
			}

			OutError = FString::Printf(TEXT("Property '%s' is not a struct or object for traversal."), *Segments[Index]);
			return false;
		}

		OutError = TEXT("Unknown property resolution failure.");
		return false;
	}

	static bool ApplyPropertyPath(UWidget* Widget, const FString& PropertyPath, const FString& PropertyValue, FString& OutError)
	{
		if (!Widget)
		{
			OutError = TEXT("Widget was null while applying property.");
			return false;
		}

		TArray<FString> Segments;
		PropertyPath.ParseIntoArray(Segments, TEXT("."), true);
		if (Segments.Num() == 0)
		{
			OutError = TEXT("Property path was empty.");
			return false;
		}

		bool bSlotRoot = false;
		if (Segments.Num() > 0 && Segments[0].Equals(TEXT("Slot"), ESearchCase::IgnoreCase))
		{
			bSlotRoot = true;
			Segments.RemoveAt(0);
		}

		UObject* Root = bSlotRoot ? Cast<UObject>(Widget->Slot) : Cast<UObject>(Widget);
		if (!Root)
		{
			OutError = bSlotRoot ? TEXT("Slot was null; widget has no panel parent.") : TEXT("Widget object invalid.");
			return false;
		}

		void* ContainerPtr = nullptr;
		FProperty* Property = nullptr;
		if (!ResolvePropertyPath(Root, Segments, ContainerPtr, Property, OutError))
		{
			return false;
		}

		if (!Property || !ContainerPtr)
		{
			OutError = TEXT("Failed to resolve property container.");
			return false;
		}

		const void* OwnerObject = Root;
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
		if (!ValuePtr)
		{
			OutError = TEXT("Failed to resolve property value pointer.");
			return false;
		}

		const TCHAR* ImportResult = Property->ImportText(*PropertyValue, ValuePtr, 0, Cast<UObject>(OwnerObject));
		if (!ImportResult)
		{
			OutError = FString::Printf(TEXT("Import failed for property '%s'."), *PropertyPath);
			return false;
		}

		return true;
	}
}

FSOTS_BPGenEnsureResult USOTS_BPGenEnsure::EnsureFunction(const UObject* WorldContextObject, const FString& BlueprintAssetPath, const FString& FunctionName, const FSOTS_BPGenFunctionSignature& Signature, bool bCreateIfMissing, bool bUpdateIfExists)
{
	FSOTS_BPGenEnsureResult Result;
	Result.BlueprintPath = BlueprintAssetPath;
	Result.Name = FunctionName;
	Result.ChangeSummary.BlueprintAssetPath = BlueprintAssetPath;
	Result.ChangeSummary.TargetType = TEXT("Function");
	Result.ChangeSummary.TargetName = FunctionName;

	if (BlueprintAssetPath.IsEmpty() || FunctionName.IsEmpty())
	{
		Result.ErrorCode = ErrorInvalidInput;
		Result.ErrorMessage = TEXT("BlueprintAssetPath or FunctionName was empty.");
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "EnsureFunction", "SOTS BPGen: Ensure Function"));

	FSOTS_BPGenGraphTarget Target;
	Target.BlueprintAssetPath = BlueprintAssetPath;
	Target.TargetType = TEXT("Function");
	Target.Name = FunctionName;
	Target.bCreateIfMissing = bCreateIfMissing;

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	FString ResolveError;
	FString ResolveErrorCode;
	if (!USOTS_BPGenGraphResolver::ResolveTargetGraph(Blueprint, FunctionGraph, Target, ResolveError, ResolveErrorCode))
	{
		Result.ErrorCode = ResolveErrorCode.IsEmpty() ? ErrorTargetNotFound : ResolveErrorCode;
		Result.ErrorMessage = ResolveError;
		return Result;
	}

	const bool bExists = FunctionGraph != nullptr;
	if (bExists && !bUpdateIfExists)
	{
		Result.bSuccess = true;
		return Result;
	}

	FSOTS_BPGenFunctionDef FunctionDef;
	FunctionDef.TargetBlueprintPath = BlueprintAssetPath;
	FunctionDef.FunctionName = FName(*FunctionName);
	FunctionDef.Inputs = Signature.Inputs;
	FunctionDef.Outputs = Signature.Outputs;

	FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyFunctionSkeleton(WorldContextObject, FunctionDef);
	Result.Warnings.Append(ApplyResult.Warnings);

	if (!ApplyResult.bSuccess)
	{
		Result.ErrorCode = !ApplyResult.ErrorCode.IsEmpty() ? ApplyResult.ErrorCode : ErrorTargetCreateFailed;
		Result.ErrorMessage = ApplyResult.ErrorMessage;
		return Result;
	}

	// Apply pure/const flags on the entry node to match the requested signature.
	if (FunctionGraph)
	{
		UK2Node_FunctionEntry* EntryNode = nullptr;
		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
			{
				EntryNode = AsEntry;
				break;
			}
		}

		if (EntryNode)
		{
			EntryNode->Modify();
			const uint32 FlagMask = FUNC_BlueprintPure | FUNC_Const;
			uint32 FlagsToApply = 0;
			if (Signature.bPure)
			{
				FlagsToApply |= FUNC_BlueprintPure;
			}
			if (Signature.bConst)
			{
				FlagsToApply |= FUNC_Const;
			}

			EntryNode->ExtraFlags &= ~FlagMask;
			EntryNode->ExtraFlags |= FlagsToApply;
		}
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	FString SaveError;
	if (!SaveBlueprint(Blueprint, SaveError))
	{
		Result.Warnings.Add(SaveError);
	}

	if (!bExists)
	{
		Result.ChangeSummary.CreatedFunctions.Add(FunctionName);
	}
	else if (bUpdateIfExists)
	{
		Result.ChangeSummary.UpdatedFunctions.Add(FunctionName);
	}

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenWidgetEnsureResult USOTS_BPGenEnsure::EnsureWidgetComponent(const UObject* WorldContextObject, const FSOTS_BPGenWidgetSpec& Request)
{
	FSOTS_BPGenWidgetEnsureResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.WidgetName = Request.WidgetName;
	Result.ParentName = Request.ParentName;
	Result.ChangeSummary.BlueprintAssetPath = Request.BlueprintAssetPath;
	Result.ChangeSummary.TargetType = TEXT("Widget");
	Result.ChangeSummary.TargetName = Request.WidgetName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.WidgetName.IsEmpty() || Request.WidgetClassPath.IsEmpty())
	{
		Result.ErrorCode = ErrorInvalidInput;
		Result.ErrorMessage = TEXT("BlueprintAssetPath, widget_name, and widget_class_path are required.");
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "EnsureWidget", "SOTS BPGen: Ensure Widget"));

	UWidgetBlueprint* WidgetBlueprint = nullptr;
	FString LoadError;
	FString LoadErrorCode;
	if (!TryLoadWidgetBlueprint(Request.BlueprintAssetPath, WidgetBlueprint, LoadError, LoadErrorCode))
	{
		Result.ErrorCode = LoadErrorCode;
		Result.ErrorMessage = LoadError;
		return Result;
	}

	UClass* WidgetClass = ResolveWidgetClass(Request.WidgetClassPath);
	if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		Result.ErrorCode = ErrorWidgetClassInvalid;
		Result.ErrorMessage = FString::Printf(TEXT("Widget class '%s' is invalid."), *Request.WidgetClassPath);
		return Result;
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint);
	}

	UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
	UWidget* TargetWidget = WidgetTree->FindWidget(FName(*Request.WidgetName));
	const bool bExists = TargetWidget != nullptr;

	if (!bExists && !Request.bCreateIfMissing)
	{
		Result.ErrorCode = ErrorWidgetNotFound;
		Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' not found and create_if_missing=false."), *Request.WidgetName);
		return Result;
	}

	if (bExists && !Request.bUpdateIfExists)
	{
		Result.bSuccess = true;
		Result.SlotType = TargetWidget && TargetWidget->Slot ? TargetWidget->Slot->GetClass()->GetName() : TEXT("");
		return Result;
	}

	if (!TargetWidget)
	{
		TargetWidget = WidgetTree->ConstructWidget<UWidget>(WidgetClass, FName(*Request.WidgetName));
		if (!TargetWidget)
		{
			Result.ErrorCode = ErrorTargetCreateFailed;
			Result.ErrorMessage = FString::Printf(TEXT("Failed to create widget of class '%s'."), *Request.WidgetClassPath);
			return Result;
		}

		Result.bCreated = true;
	}

	UPanelWidget* ParentPanel = nullptr;
	if (!Request.ParentName.IsEmpty())
	{
		UWidget* ParentWidget = WidgetTree->FindWidget(FName(*Request.ParentName));
		if (!ParentWidget)
		{
			Result.ErrorCode = ErrorWidgetParentInvalid;
			Result.ErrorMessage = FString::Printf(TEXT("Parent widget '%s' not found."), *Request.ParentName);
			return Result;
		}

		ParentPanel = Cast<UPanelWidget>(ParentWidget);
		if (!ParentPanel)
		{
			Result.ErrorCode = ErrorWidgetParentInvalid;
			Result.ErrorMessage = FString::Printf(TEXT("Parent widget '%s' is not a panel widget."), *Request.ParentName);
			return Result;
		}
	}

	bool bReparented = false;
	if (ParentPanel)
	{
		if (UPanelWidget* CurrentParent = Cast<UPanelWidget>(TargetWidget->GetParent()))
		{
			if (CurrentParent != ParentPanel)
			{
				if (!Request.bReparentIfMismatch)
				{
					Result.ErrorCode = ErrorWidgetParentInvalid;
					Result.ErrorMessage = TEXT("Widget already has a different parent and reparent_if_mismatch=false.");
					return Result;
				}

				CurrentParent->RemoveChild(TargetWidget);
				bReparented = true;
			}
		}
		else if (WidgetTree->RootWidget == TargetWidget)
		{
			if (!Request.bReparentIfMismatch)
			{
				Result.ErrorCode = ErrorWidgetParentInvalid;
				Result.ErrorMessage = TEXT("Widget is root and reparent_if_mismatch=false.");
				return Result;
			}

			WidgetTree->RootWidget = nullptr;
			bReparented = true;
		}

		if (TargetWidget->GetParent() != ParentPanel)
		{
			if (Request.InsertIndex >= 0)
			{
				const int32 InsertAt = FMath::Clamp(Request.InsertIndex, 0, ParentPanel->GetChildrenCount());
				ParentPanel->InsertChildAt(InsertAt, TargetWidget);
			}
			else
			{
				ParentPanel->AddChild(TargetWidget);
			}
		}
		else if (Request.InsertIndex >= 0)
		{
			const int32 Desired = FMath::Clamp(Request.InsertIndex, 0, ParentPanel->GetChildrenCount() - 1);
			const int32 CurrentIndex = ParentPanel->GetChildIndex(TargetWidget);
			if (CurrentIndex != Desired && CurrentIndex != INDEX_NONE)
			{
				ParentPanel->RemoveChild(TargetWidget);
				ParentPanel->InsertChildAt(Desired, TargetWidget);
				bReparented = true;
			}
		}
	}
	else
	{
		if (!WidgetTree->RootWidget)
		{
			WidgetTree->RootWidget = TargetWidget;
		}
		else if (WidgetTree->RootWidget != TargetWidget && Request.bReparentIfMismatch)
		{
			if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(TargetWidget->GetParent()))
			{
				ExistingParent->RemoveChild(TargetWidget);
			}
			WidgetTree->RootWidget = TargetWidget;
			bReparented = true;
		}
	}

	Result.bReparented = bReparented;
	Result.SlotType = TargetWidget->Slot ? TargetWidget->Slot->GetClass()->GetName() : TEXT("");

	if (Request.bIsVariable)
	{
		TargetWidget->bIsVariable = true;
	}

	TargetWidget->Modify();

	for (const TPair<FString, FString>& Pair : Request.Properties)
	{
		FString PropError;
		if (ApplyPropertyPath(TargetWidget, Pair.Key, Pair.Value, PropError))
		{
			Result.AppliedProperties.Add(Pair.Key);
		}
		else
		{
			Result.Warnings.Add(FString::Printf(TEXT("Property '%s' skipped: %s"), *Pair.Key, *PropError));
		}
	}

	for (const TPair<FString, FString>& Pair : Request.SlotProperties)
	{
		FString PropError;
		const FString SlotPath = FString::Printf(TEXT("Slot.%s"), *Pair.Key);
		if (ApplyPropertyPath(TargetWidget, SlotPath, Pair.Value, PropError))
		{
			Result.AppliedProperties.Add(SlotPath);
		}
		else
		{
			Result.Warnings.Add(FString::Printf(TEXT("Slot property '%s' skipped: %s"), *Pair.Key, *PropError));
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	FString SaveError;
	if (!SaveBlueprint(WidgetBlueprint, SaveError))
	{
		Result.Warnings.Add(SaveError);
	}

	if (Result.bCreated)
	{
		Result.ChangeSummary.CreatedWidgets.Add(Request.WidgetName);
	}
	else if (Result.bReparented || Result.AppliedProperties.Num() > 0)
	{
		Result.ChangeSummary.UpdatedWidgets.Add(Request.WidgetName);
	}
	Result.ChangeSummary.PropertySets = Result.AppliedProperties;

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenWidgetPropertyResult USOTS_BPGenEnsure::SetWidgetProperties(const UObject* WorldContextObject, const FSOTS_BPGenWidgetPropertyRequest& Request)
{
	FSOTS_BPGenWidgetPropertyResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.WidgetName = Request.WidgetName;
	Result.ChangeSummary.BlueprintAssetPath = Request.BlueprintAssetPath;
	Result.ChangeSummary.TargetType = TEXT("WidgetProperty");
	Result.ChangeSummary.TargetName = Request.WidgetName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.WidgetName.IsEmpty())
	{
		Result.ErrorCode = ErrorInvalidInput;
		Result.ErrorMessage = TEXT("BlueprintAssetPath and widget_name are required.");
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "SetWidgetProperties", "SOTS BPGen: Set Widget Properties"));

	UWidgetBlueprint* WidgetBlueprint = nullptr;
	FString LoadError;
	FString LoadErrorCode;
	if (!TryLoadWidgetBlueprint(Request.BlueprintAssetPath, WidgetBlueprint, LoadError, LoadErrorCode))
	{
		Result.ErrorCode = LoadErrorCode;
		Result.ErrorMessage = LoadError;
		return Result;
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		Result.ErrorCode = ErrorWidgetNotFound;
		Result.ErrorMessage = TEXT("Widget blueprint has no widget tree.");
		return Result;
	}

	UWidget* TargetWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*Request.WidgetName));
	if (!TargetWidget)
	{
		if (Request.bFailIfMissing)
		{
			Result.ErrorCode = ErrorWidgetNotFound;
			Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' not found."), *Request.WidgetName);
			return Result;
		}

		Result.Warnings.Add(FString::Printf(TEXT("Widget '%s' not found; no properties applied."), *Request.WidgetName));
		Result.bSuccess = true;
		return Result;
	}

	TargetWidget->Modify();

	for (const TPair<FString, FString>& Pair : Request.Properties)
	{
		FString PropError;
		if (ApplyPropertyPath(TargetWidget, Pair.Key, Pair.Value, PropError))
		{
			Result.AppliedProperties.Add(Pair.Key);
		}
		else
		{
			Result.Warnings.Add(FString::Printf(TEXT("Property '%s' skipped: %s"), *Pair.Key, *PropError));
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	FString SaveError;
	if (!SaveBlueprint(WidgetBlueprint, SaveError))
	{
		Result.Warnings.Add(SaveError);
	}

	if (Result.AppliedProperties.Num() > 0)
	{
		Result.ChangeSummary.UpdatedWidgets.Add(Request.WidgetName);
		Result.ChangeSummary.PropertySets = Result.AppliedProperties;
	}

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenBindingEnsureResult USOTS_BPGenEnsure::EnsureWidgetBinding(const UObject* WorldContextObject, const FSOTS_BPGenBindingRequest& Request)
{
	FSOTS_BPGenBindingEnsureResult Result;
	Result.BlueprintPath = Request.BlueprintAssetPath;
	Result.WidgetName = Request.WidgetName;
	Result.PropertyName = Request.PropertyName;
	Result.FunctionName = Request.FunctionName;
	Result.ChangeSummary.BlueprintAssetPath = Request.BlueprintAssetPath;
	Result.ChangeSummary.TargetType = TEXT("WidgetBinding");
	Result.ChangeSummary.TargetName = Request.FunctionName;

	if (Request.BlueprintAssetPath.IsEmpty() || Request.WidgetName.IsEmpty() || Request.PropertyName.IsEmpty() || Request.FunctionName.IsEmpty())
	{
		Result.ErrorCode = ErrorInvalidInput;
		Result.ErrorMessage = TEXT("BlueprintAssetPath, widget_name, property_name, and function_name are required.");
		return Result;
	}

	FSOTS_BPGenApplyResult ApplyResult;
	bool bAppliedGraph = false;

	// Ensure the binding function exists first (owns its own lock/transaction).
	if (Request.bCreateFunction || Request.bUpdateFunction)
	{
		FSOTS_BPGenEnsureResult FunctionResult = USOTS_BPGenEnsure::EnsureFunction(
			WorldContextObject,
			Request.BlueprintAssetPath,
			Request.FunctionName,
			Request.Signature,
			Request.bCreateFunction,
			Request.bUpdateFunction);

		Result.Warnings.Append(FunctionResult.Warnings);
		Result.bFunctionCreated = FunctionResult.bSuccess && Request.bCreateFunction;
		Result.bFunctionUpdated = FunctionResult.bSuccess && Request.bUpdateFunction;

		if (!FunctionResult.bSuccess)
		{
			Result.ErrorCode = FunctionResult.ErrorCode;
			Result.ErrorMessage = FunctionResult.ErrorMessage;
			return Result;
		}
	}

	// Update binding entry (scoped lock to avoid double-locking inside EnsureFunction/ApplyGraph).
	{
		FScopeLock Lock(&GSOTS_BPGenEditMutex);
		FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "EnsureWidgetBinding", "SOTS BPGen: Ensure Widget Binding"));

		UWidgetBlueprint* WidgetBlueprint = nullptr;
		FString LoadError;
		FString LoadErrorCode;
		if (!TryLoadWidgetBlueprint(Request.BlueprintAssetPath, WidgetBlueprint, LoadError, LoadErrorCode))
		{
			Result.ErrorCode = LoadErrorCode;
			Result.ErrorMessage = LoadError;
			return Result;
		}

		if (!WidgetBlueprint->WidgetTree)
		{
			Result.ErrorCode = ErrorWidgetNotFound;
			Result.ErrorMessage = TEXT("Widget blueprint has no widget tree.");
			return Result;
		}

		UWidget* TargetWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*Request.WidgetName));
		if (!TargetWidget)
		{
			Result.ErrorCode = ErrorWidgetNotFound;
			Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' not found."), *Request.WidgetName);
			return Result;
		}

		// Validate property exists (non-fatal warning if not found).
		{
			TArray<FString> Segments;
			Request.PropertyName.ParseIntoArray(Segments, TEXT("."), true);
			void* ContainerPtr = nullptr;
			FProperty* Property = nullptr;
			FString PropError;
			if (!ResolvePropertyPath(TargetWidget, Segments, ContainerPtr, Property, PropError))
			{
				Result.Warnings.Add(FString::Printf(TEXT("Property '%s' not resolved: %s"), *Request.PropertyName, *PropError));
			}
		}

		TArray<FDelegateEditorBinding>& Bindings = WidgetBlueprint->Bindings;
		const FName ObjectFName(*Request.WidgetName);
		const FName PropertyFName(*Request.PropertyName);
		const FName FunctionFName(*Request.FunctionName);

		int32 ExistingIndex = Bindings.IndexOfByPredicate([&](const FDelegateEditorBinding& Binding)
		{
			return Binding.ObjectName == ObjectFName && Binding.PropertyName == PropertyFName;
		});

		if (ExistingIndex != INDEX_NONE)
		{
			if (!Request.bUpdateBinding)
			{
				Result.Warnings.Add(TEXT("Binding already exists; update_binding=false so leaving as-is."));
			}
			else
			{
				Bindings[ExistingIndex].FunctionName = FunctionFName;
				Result.bBindingUpdated = true;
			}
		}
		else
		{
			if (!Request.bCreateBinding)
			{
				Result.ErrorCode = ErrorTargetNotFound;
				Result.ErrorMessage = TEXT("Binding not found and create_binding=false.");
				return Result;
			}

			FDelegateEditorBinding& NewBinding = Bindings.AddDefaulted_GetRef();
			NewBinding.ObjectName = ObjectFName;
			NewBinding.PropertyName = PropertyFName;
			NewBinding.FunctionName = FunctionFName;
			Result.bBindingCreated = true;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		FString SaveError;
		if (!SaveBlueprint(WidgetBlueprint, SaveError))
		{
			Result.Warnings.Add(SaveError);
		}
	}

	// Apply graph spec after binding edits so the binding function body matches.
	const bool bHasGraph = Request.bApplyGraph || Request.GraphSpec.Nodes.Num() > 0 || Request.GraphSpec.Links.Num() > 0;
	if (bHasGraph)
	{
		FSOTS_BPGenGraphSpec GraphSpec = Request.GraphSpec;
		GraphSpec.Target.BlueprintAssetPath = GraphSpec.Target.BlueprintAssetPath.IsEmpty() ? Request.BlueprintAssetPath : GraphSpec.Target.BlueprintAssetPath;
		GraphSpec.Target.Name = GraphSpec.Target.Name.IsEmpty() ? Request.FunctionName : GraphSpec.Target.Name;
		GraphSpec.Target.TargetType = GraphSpec.Target.TargetType.IsEmpty() ? TEXT("WidgetBinding") : GraphSpec.Target.TargetType;
		GraphSpec.Target.bCreateIfMissing = true;

		ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(WorldContextObject, GraphSpec);
		bAppliedGraph = true;
		Result.Warnings.Append(ApplyResult.Warnings);
		if (!ApplyResult.bSuccess)
		{
			Result.ErrorCode = ApplyResult.ErrorCode;
			Result.ErrorMessage = ApplyResult.ErrorMessage;
			return Result;
		}
	}

	if (Result.bBindingCreated)
	{
		Result.ChangeSummary.BindingsCreated.Add(Request.WidgetName + TEXT(".") + Request.PropertyName);
	}
	if (Result.bFunctionCreated)
	{
		Result.ChangeSummary.CreatedFunctions.Add(Request.FunctionName);
	}
	if (Result.bFunctionUpdated)
	{
		Result.ChangeSummary.UpdatedFunctions.Add(Request.FunctionName);
	}
	if (bAppliedGraph)
	{
		Result.ChangeSummary.CreatedNodeIds = ApplyResult.CreatedNodeIds;
		Result.ChangeSummary.UpdatedNodeIds = ApplyResult.UpdatedNodeIds;
	}

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenEnsureResult USOTS_BPGenEnsure::EnsureVariable(const UObject* WorldContextObject, const FString& BlueprintAssetPath, const FString& VarName, const FSOTS_BPGenPin& VarType, const FString& DefaultValue, bool bInstanceEditable, bool bExposeOnSpawn, bool bCreateIfMissing, bool bUpdateIfExists)
{
	FSOTS_BPGenEnsureResult Result;
	Result.BlueprintPath = BlueprintAssetPath;
	Result.Name = VarName;
	Result.ChangeSummary.BlueprintAssetPath = BlueprintAssetPath;
	Result.ChangeSummary.TargetType = TEXT("Variable");
	Result.ChangeSummary.TargetName = VarName;

	if (BlueprintAssetPath.IsEmpty() || VarName.IsEmpty())
	{
		Result.ErrorCode = ErrorInvalidInput;
		Result.ErrorMessage = TEXT("BlueprintAssetPath or VarName was empty.");
		return Result;
	}

	FScopeLock Lock(&GSOTS_BPGenEditMutex);
	FScopedTransaction Transaction(NSLOCTEXT("SOTS_BPGen", "EnsureVariable", "SOTS BPGen: Ensure Variable"));

	FSOTS_BPGenGraphTarget Target;
	Target.BlueprintAssetPath = BlueprintAssetPath;
	Target.TargetType = TEXT("Function"); // Only to load the Blueprint
	Target.Name = VarName;
	Target.bCreateIfMissing = true;

	UBlueprint* Blueprint = nullptr;
	UEdGraph* DummyGraph = nullptr;
	FString ResolveError;
	FString ResolveErrorCode;
	if (!USOTS_BPGenGraphResolver::ResolveTargetGraph(Blueprint, DummyGraph, Target, ResolveError, ResolveErrorCode))
	{
		Result.ErrorCode = ResolveErrorCode.IsEmpty() ? ErrorTargetNotFound : ResolveErrorCode;
		Result.ErrorMessage = ResolveError;
		return Result;
	}

	const FName VarFName(*VarName);
	const bool bExists = Blueprint->NewVariables.ContainsByPredicate([&](const FBPVariableDescription& Desc){ return Desc.VarName == VarFName; });

	if (!bExists && !bCreateIfMissing)
	{
		Result.ErrorCode = ErrorTargetNotFound;
		Result.ErrorMessage = FString::Printf(TEXT("Variable '%s' not found and create_if_missing=false."), *VarName);
		return Result;
	}

	if (bExists && !bUpdateIfExists)
	{
		Result.bSuccess = true;
		return Result;
	}

	FEdGraphPinType PinType;
	if (!MakePinTypeFromSpec(VarType, PinType))
	{
		Result.ErrorCode = ErrorTypeInvalid;
		Result.ErrorMessage = TEXT("Variable type was invalid (missing category).");
		return Result;
	}

	Blueprint->Modify();
	if (!bExists)
	{
		if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarFName, PinType, DefaultValue))
		{
			Result.ErrorCode = ErrorTargetCreateFailed;
			Result.ErrorMessage = FString::Printf(TEXT("Failed to create variable '%s'."), *VarName);
			return Result;
		}
	}
	else
	{
		if (!FBlueprintEditorUtils::ChangeMemberVariableType(Blueprint, VarFName, PinType))
		{
			Result.ErrorCode = ErrorTypeInvalid;
			Result.ErrorMessage = FString::Printf(TEXT("Failed to update variable '%s' type."), *VarName);
			return Result;
		}

		if (!DefaultValue.IsEmpty())
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VarFName, nullptr, FBlueprintMetadata::MD_DefaultValue, *DefaultValue);
		}
	}

	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VarFName, nullptr, FBlueprintMetadata::MD_ExposeOnSpawn, bExposeOnSpawn ? TEXT("true") : TEXT("false"));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VarFName, nullptr, FBlueprintMetadata::MD_EditInline, bInstanceEditable ? TEXT("true") : TEXT("false"));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	FString SaveError;
	if (!SaveBlueprint(Blueprint, SaveError))
	{
		Result.Warnings.Add(SaveError);
	}

	Result.bSuccess = true;
	if (!bExists)
	{
		Result.ChangeSummary.CreatedVariables.Add(VarName);
	}
	else
	{
		Result.ChangeSummary.UpdatedVariables.Add(VarName);
	}
	return Result;
}
