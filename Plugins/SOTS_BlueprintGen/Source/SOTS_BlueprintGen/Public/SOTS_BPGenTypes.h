#pragma once

#include "CoreMinimal.h"
#include "SOTS_BPGenTypes.generated.h"

/**
 * Direction of a generated pin (input vs output).
 */
UENUM(BlueprintType)
enum class ESOTS_BPGenPinDirection : uint8
{
	Input  UMETA(DisplayName = "Input"),
	Output UMETA(DisplayName = "Output")
};

/**
 * Container type for a generated pin.
 */
UENUM(BlueprintType)
enum class ESOTS_BPGenContainerType : uint8
{
	None  UMETA(DisplayName = "None"),
	Array UMETA(DisplayName = "Array"),
	Set   UMETA(DisplayName = "Set"),
	Map   UMETA(DisplayName = "Map")
};

/**
 * Blueprint generation pin description.
 *
 * This is an abstract description used by DevTools JSON and the
 * USOTS_BPGenBuilder. It does NOT yet map directly to FEdGraphPinType;
 * that will be added in a later pass.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenPin
{
	GENERATED_BODY()

	/** JSON: "Name" (required for struct members/variables; optional for some graph nodes). Logical pin name such as AbilityData or NameText. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName Name;

	/** JSON: "Category" (required). Main pin category like "bool", "int", "float", "struct", or "object". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName Category;

	/** JSON: "SubCategory" (optional). Sub-category such as "byte", "int64", or "SoftObject"; empty for most primitive pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName SubCategory;

	/** JSON: "SubObjectPath" (optional). Asset/class path used when Category/SubCategory requires an object or struct. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FString SubObjectPath;

	/** JSON: "ContainerType" (optional, defaults to "None"). Valid: None|Array|Set|Map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	ESOTS_BPGenContainerType ContainerType = ESOTS_BPGenContainerType::None;

	/** JSON: "Direction" (optional, defaults to "Input"). Valid: Input|Output. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	ESOTS_BPGenPinDirection Direction = ESOTS_BPGenPinDirection::Input;

	/** JSON: "DefaultValue" (optional). Literal default value as string for compatible pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FString DefaultValue;
};

/**
 * Definition of a struct asset to generate.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenStructDef
{
	GENERATED_BODY()

	/** JSON: "AssetPath" (required). Long package path for the struct asset (e.g. "/Game/SOTS/Generated/Structs/F_SOTS_Example"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FString AssetPath;

	/** JSON: "StructName" (optional; defaults to the object name parsed from AssetPath if omitted/None). Usually matches asset name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName StructName;

	/** JSON: "Members" (required, array). Each entry is FSOTS_BPGenPin describing a member variable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	TArray<FSOTS_BPGenPin> Members;
};

/**
 * Definition of an enum asset to generate.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenEnumDef
{
	GENERATED_BODY()

	/** JSON: "AssetPath" (required). Long package path for the enum asset (e.g. "/Game/SOTS/Generated/Enums/E_SOTS_Example"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FString AssetPath;

	/** JSON: "EnumName" (optional; defaults to object name parsed from AssetPath if omitted/None). Usually matches asset name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName EnumName;

	/** JSON: "Values" (required, array of strings). Each entry becomes an enum value in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	TArray<FString> Values;
};

/**
 * Definition of a function to generate inside an existing Blueprint.
 *
 * The graph body will be described separately via FSOTS_BPGenGraphSpec.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenFunctionDef
{
	GENERATED_BODY()

	/** JSON: "TargetBlueprintPath" (required). Full object path to the target Blueprint asset (e.g. "/Game/Dev/BP_Sample.BP_Sample"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FString TargetBlueprintPath;

	/** JSON: "FunctionName" (required). Name of the function to create or update. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	FName FunctionName;

	/** JSON: "Inputs" (optional, array). FSOTS_BPGenPin definitions for input parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	TArray<FSOTS_BPGenPin> Inputs;

	/** JSON: "Outputs" (optional, array). FSOTS_BPGenPin definitions for output parameters/return values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen")
	TArray<FSOTS_BPGenPin> Outputs;
};

/**
 * A node inside a generated function graph.
 *
 * DevTools JSON will map directly to this shape.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphNode
{
	GENERATED_BODY()

	/** JSON: "Id" (required). Unique identifier within a single graph spec; used by links. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString Id;

	/**
	 * Node type identifier (e.g. "K2Node_CallFunction", "K2Node_VariableGet").
	 * This will be interpreted by the builder to choose the correct UK2Node subclass.
	 */
	/** JSON: "NodeType" (required). Name of the K2 node class to spawn/resolve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName NodeType;

	/**
	 * Optional function path for call-function nodes
	 * (e.g. "/Script/Engine.KismetMathLibrary:Clamp").
	 */
	/** JSON: "FunctionPath" (optional; required by call-function/array-function nodes). Full function path if NodeType needs it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString FunctionPath;

	/**
	 * Optional struct path for struct nodes (e.g. "/Script/Engine.Vector").
	 */
	/** JSON: "StructPath" (optional; required by make/break struct nodes). Full struct path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString StructPath;

	/**
	 * Optional variable name for variable nodes
	 * (e.g. "AbilityData", "Health", etc.).
	 */
	/** JSON: "VariableName" (optional; required by variable get/set, event/custom event names, select option pins, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName VariableName;

	/** JSON: "NodePosition" (optional; defaults to 0,0). Graph editor position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FVector2D NodePosition = FVector2D::ZeroVector;

	/**
	 * Extra key/value data for this node. This can be used for
	 * specialized settings (timeline lengths, meta flags, etc.).
	 */
	/** JSON: "ExtraData" (optional, map<string,string>). Free-form pairs for node-specific defaults (pin values, select options, container types, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TMap<FName, FString> ExtraData;
};

/**
 * A directed connection between two node pins.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphLink
{
	GENERATED_BODY()

	/** JSON: "FromNodeId" (required). Id of the source node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString FromNodeId;

	/** JSON: "FromPinName" (required). Pin name on the source node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName FromPinName;

	/** JSON: "ToNodeId" (required). Id of the target node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString ToNodeId;

	/** JSON: "ToPinName" (required). Pin name on the target node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName ToPinName;
};

/**
 * Full graph specification for a single function body.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphSpec
{
	GENERATED_BODY()

	/** JSON: "Nodes" (optional, array). Each entry is an FSOTS_BPGenGraphNode; empty array means no additional nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TArray<FSOTS_BPGenGraphNode> Nodes;

	/** JSON: "Links" (optional, array). Each entry is an FSOTS_BPGenGraphLink connecting node pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TArray<FSOTS_BPGenGraphLink> Links;
};

/**
 * Result for asset-creation operations (structs, enums).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenAssetResult
{
	GENERATED_BODY()

	/** JSON: "bSuccess" (output only). True if struct/enum create/update succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	bool bSuccess = false;

	/** JSON: "AssetPath" (output only). Path of the asset processed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString AssetPath;

	/** JSON: "Message" (output only). Human-readable info or error text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString Message;
};

/**
 * Result for function/graph-application operations.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenApplyResult
{
	GENERATED_BODY()

	/** JSON: "bSuccess" (output only). True if the apply operation succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	bool bSuccess = false;

	/** JSON: "TargetBlueprintPath" (output only). Blueprint path that was processed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString TargetBlueprintPath;

	/** JSON: "FunctionName" (output only). Function name that was affected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FName FunctionName;

	/** JSON: "ErrorMessage" (output only). Populated when bSuccess is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString ErrorMessage;

	/** JSON: "Warnings" (output only, array). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> Warnings;
};

// Canonical node type names used by BPGen graph specs.
namespace SOTS_BPGenNodeTypes
{
	inline const FName FunctionEntry(TEXT("K2Node_FunctionEntry"));
	inline const FName FunctionResult(TEXT("K2Node_FunctionResult"));
	inline const FName Knot(TEXT("K2Node_Knot"));
	inline const FName EnumLiteral(TEXT("K2Node_EnumLiteral"));
	inline const FName MakeStruct(TEXT("K2Node_MakeStruct"));
	inline const FName BreakStruct(TEXT("K2Node_BreakStruct"));
	inline const FName Select(TEXT("K2Node_Select"));
	inline const FName Event(TEXT("K2Node_Event"));
	inline const FName CustomEvent(TEXT("K2Node_CustomEvent"));
}
