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

	/** JSON: "SpawnerKey" (optional). Stable spawner identifier; if set and bPreferSpawnerKey is true, spawn via spawner registry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString SpawnerKey;

	/** JSON: "PreferSpawnerKey" (optional, defaults true). When false, skip spawner path even if SpawnerKey is provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bPreferSpawnerKey = true;

	/** JSON: "NodeId" (optional). Stable identifier to support create-or-update flows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString NodeId;

	/** JSON: "create_or_update" (optional, defaults true). When false, always create new even if NodeId matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bCreateOrUpdate = true;

	/** JSON: "allow_create" (optional, defaults true). If false and NodeId missing, node will be skipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bAllowCreate = true;

	/** JSON: "allow_update" (optional, defaults true). If false, existing nodes with NodeId will be left untouched. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bAllowUpdate = true;

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

	/** JSON: "break_existing_from" (optional). When true, break existing links on the source pin before connecting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bBreakExistingFrom = false;

	/** JSON: "break_existing_to" (optional). When true, break existing links on the target pin before connecting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bBreakExistingTo = false;

	/** JSON: "use_schema" (optional, defaults true). When true, attempt schema TryCreateConnection before fallback MakeLinkTo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bUseSchema = true;

	/** JSON: "allow_heuristic_pin_match" (optional, defaults false). When true, allow case-insensitive + alias pin matching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bAllowHeuristicPinMatch = false;
};

/**
 * Auto-fix step recorded during graph apply (SPINE_J).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenAutoFixStep
{
	GENERATED_BODY()

	/** JSON: "step_index" (output). Ordered step index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	int32 StepIndex = 0;

	/** JSON: "code" (output). Stable fix code (e.g., FIX_PIN_ALIAS). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	FString Code;

	/** JSON: "description" (output). Human-readable summary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	FString Description;

	/** JSON: "affected_node_ids" (output). Node ids touched by the fix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	TArray<FString> AffectedNodeIds;

	/** JSON: "affected_pins" (output). Pin identifiers involved in the fix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	TArray<FString> AffectedPins;

	/** JSON: "before" (output, optional). Before snapshot snippet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	FString Before;

	/** JSON: "after" (output, optional). After snapshot snippet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|AutoFix")
	FString After;
};

/**
 * Repair step recorded during graph apply (SPINE_M).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenRepairStep
{
	GENERATED_BODY()

	/** JSON: "step_index" (output). Ordered step index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair", meta = (JsonKey = "step_index"))
	int32 StepIndex = 0;

	/** JSON: "code" (output). Stable repair code (e.g., REPAIR_NODE_ID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair")
	FString Code;

	/** JSON: "description" (output). Human-readable summary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair")
	FString Description;

	/** JSON: "affected_node_ids" (output). Node ids touched by the repair. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair", meta = (JsonKey = "affected_node_ids"))
	TArray<FString> AffectedNodeIds;

	/** JSON: "before" (output, optional). Before snapshot snippet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair")
	FString Before;

	/** JSON: "after" (output, optional). After snapshot snippet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Repair")
	FString After;
};

/**
 * Delete-node request targeting a specific graph.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenDeleteNodeRequest
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString BlueprintAssetPath;

	/** JSON: "function_name" (required). Function or graph name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName FunctionName;

	/** JSON: "node_id" (required). Stable node id (NodeComment/NodeGuid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString NodeId;

	/** JSON: "compile" (optional, default true). Compile after mutation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bCompile = true;
};

/**
 * Delete-link request between two pins.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenDeleteLinkRequest
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString BlueprintAssetPath;

	/** JSON: "function_name" (required). Function or graph name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName FunctionName;

	/** JSON: "from_node_id" (required). Source node id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString FromNodeId;

	/** JSON: "from_pin" (required). Source pin name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName FromPinName;

	/** JSON: "to_node_id" (required). Target node id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString ToNodeId;

	/** JSON: "to_pin" (required). Target pin name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName ToPinName;

	/** JSON: "compile" (optional, default true). Compile after mutation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bCompile = true;
};

/**
 * Replace-node request that preserves NodeId while swapping implementation.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenReplaceNodeRequest
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString BlueprintAssetPath;

	/** JSON: "function_name" (required). Function or graph name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FName FunctionName;

	/** JSON: "existing_node_id" (required). NodeId to replace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString ExistingNodeId;

	/** JSON: "new_node" (required). New node spec (SpawnerKey/NodeType/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FSOTS_BPGenGraphNode NewNode;

	/** JSON: "pin_remap" (optional). Map of old pin names to new pin names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TMap<FName, FName> PinRemap;

	/** JSON: "compile" (optional, default true). Compile after mutation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bCompile = true;
};

/**
 * Generic graph edit result used by delete/replace helpers.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphEditResult
{
	GENERATED_BODY()

	/** JSON: "b_success" (output). True if operation succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	bool bSuccess = false;

	/** JSON: "blueprint_path" (output). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString BlueprintPath;

	/** JSON: "function_name" (output). Function graph context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FName FunctionName;

	/** JSON: "message" (output). Informational message. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString Message;

	/** JSON: "errors" (output). Fatal errors encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> Errors;

	/** JSON: "warnings" (output). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> Warnings;

	/** JSON: "affected_node_ids" (output). Node ids touched by the edit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> AffectedNodeIds;
};

/**
 * Target descriptor for a graph application.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphTarget
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required for target-only requests). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString BlueprintAssetPath;

	/** JSON: "target_type" (optional; defaults to Function). Examples: Function, EventGraph, ConstructionScript, Macro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString TargetType;

	/** JSON: "name" (optional; required for most target types). FunctionName/MacroName/etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString Name;

	/** JSON: "sub_name" (optional). Additional context (state machine/state names, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FString SubName;

	/** JSON: "create_if_missing" (optional, defaults true). When false, missing targets produce an error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bCreateIfMissing = true;
};

/**
 * Full graph specification for a single function body.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenGraphSpec
{
	GENERATED_BODY()

	/** JSON: "spec_version" (optional, defaults to 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph", meta = (JsonKey = "spec_version"))
	int32 SpecVersion = 1;

	/** JSON: "spec_schema" (optional, defaults to "SOTS_BPGen_GraphSpec"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph", meta = (JsonKey = "spec_schema"))
	FString SpecSchema = TEXT("SOTS_BPGen_GraphSpec");

	/** JSON: "repair_mode" (optional, defaults to "none"). Valid: none|soft|aggressive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph", meta = (JsonKey = "repair_mode"))
	FString RepairMode;

	/** JSON: "target" (optional). Graph target descriptor; defaults to Function when omitted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	FSOTS_BPGenGraphTarget Target;

	/** JSON: "Nodes" (optional, array). Each entry is an FSOTS_BPGenGraphNode; empty array means no additional nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TArray<FSOTS_BPGenGraphNode> Nodes;

	/** JSON: "Links" (optional, array). Each entry is an FSOTS_BPGenGraphLink connecting node pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	TArray<FSOTS_BPGenGraphLink> Links;

	/** JSON: "auto_fix" (optional, defaults false). Enable auto-fix heuristics on link failures. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bAutoFix = false;

	/** JSON: "auto_fix_max_steps" (optional, defaults 5). Max auto-fix steps per apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	int32 AutoFixMaxSteps = 5;

	/** JSON: "auto_fix_insert_conversions" (optional, defaults false). Allow conversion node insertion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Graph")
	bool bAutoFixInsertConversions = false;
};

/**
 * Canonicalization options for GraphSpec (SPINE_M).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenSpecCanonicalizeOptions
{
	GENERATED_BODY()

	/** JSON: "assign_missing_node_ids" (optional, defaults true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "assign_missing_node_ids"))
	bool bAssignMissingNodeIds = true;

	/** JSON: "sort_deterministic" (optional, defaults true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "sort_deterministic"))
	bool bSortDeterministic = true;

	/** JSON: "normalize_target_type" (optional, defaults true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "normalize_target_type"))
	bool bNormalizeTargetType = true;

	/** JSON: "apply_migrations" (optional, defaults true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "apply_migrations"))
	bool bApplyMigrations = true;
};

/**
 * Canonicalization output for GraphSpec (SPINE_M).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenCanonicalizeResult
{
	GENERATED_BODY()

	/** JSON: "canonical_spec" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "canonical_spec"))
	FSOTS_BPGenGraphSpec CanonicalSpec;

	/** JSON: "diff_notes" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "diff_notes"))
	TArray<FString> DiffNotes;

	/** JSON: "spec_migrated" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "spec_migrated"))
	bool bSpecMigrated = false;

	/** JSON: "migration_notes" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Spec", meta = (JsonKey = "migration_notes"))
	TArray<FString> MigrationNotes;
};

/**
 * Discovered pin descriptor used by node discovery (SPINE_A).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenDiscoveredPinDescriptor
{
	GENERATED_BODY()

	/** JSON: "pin_name" (output). Pin name returned by discovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FName PinName;

	/** JSON: "direction" (output). Input or Output. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	ESOTS_BPGenPinDirection Direction = ESOTS_BPGenPinDirection::Input;

	/** JSON: "pin_category" (output). Pin category (e.g., bool, float, struct). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FName PinCategory;

	/** JSON: "pin_subcategory" (output). Optional subcategory such as byte, int64, SoftObject. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FName PinSubCategory;

	/** JSON: "sub_object_path" (output). Optional object/struct path associated with the pin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString SubObjectPath;

	/** JSON: "container_type" (output). None|Array|Set|Map as a string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString ContainerType;

	/** JSON: "default_value" (output). Default value literal when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString DefaultValue;

	/** JSON: "b_is_hidden" (output). True if the pin is marked hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	bool bIsHidden = false;

	/** JSON: "b_is_advanced" (output). True if the pin is advanced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	bool bIsAdvanced = false;

	/** JSON: "tooltip" (output). Pin tooltip text if available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString Tooltip;
};

/**
 * Discovered node spawner descriptor (stable spawner_key + optional pin descriptors).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeSpawnerDescriptor
{
	GENERATED_BODY()

	/** JSON: "spawner_key" (output). Stable key used to spawn the node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString SpawnerKey;

	/** JSON: "display_name" (output). Palette/display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString DisplayName;

	/** JSON: "category" (output). Palette category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString Category;

	/** JSON: "keywords" (output). Keywords associated with the node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	TArray<FString> Keywords;

	/** JSON: "tooltip" (output). Tooltip text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString Tooltip;

	/** JSON: "node_class_name" (output). K2 node class name (e.g., K2Node_CallFunction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString NodeClassName;

	/** JSON: "node_class_path" (output). Full path to the node class. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString NodeClassPath;

	/** JSON: "node_type" (output). Normalized node type string (function_call, variable_get, cast, generic, synthetic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString NodeType;

	/** JSON: "function_path" (output). UFunction path if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString FunctionPath;

	/** JSON: "struct_path" (output). UScriptStruct path if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString StructPath;

	/** JSON: "variable_name" (output). Variable name for var get/set nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FName VariableName;

	/** JSON: "variable_owner_class_path" (output). Class path owning the variable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString VariableOwnerClassPath;

	/** JSON: "variable_pin_category" (output). Pin category derived from the property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString VariablePinCategory;

	/** JSON: "variable_pin_subcategory" (output). Pin subcategory derived from the property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString VariablePinSubCategory;

	/** JSON: "variable_pin_subobject_path" (output). Pin subobject path derived from the property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString VariablePinSubObjectPath;

	/** JSON: "variable_pin_container_type" (output). Container type for the property (None|Array|Set|Map). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString VariablePinContainerType;

	/** JSON: "target_class_path" (output). Target class path for cast nodes, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString TargetClassPath;

	/** JSON: "pins" (output). Optional pin descriptors when requested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	TArray<FSOTS_BPGenDiscoveredPinDescriptor> Pins;

	/** JSON: "b_is_synthetic" (output). True if descriptor represents a synthetic helper (e.g., reroute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	bool bIsSynthetic = false;
};

/**
 * Result for node discovery queries.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeDiscoveryResult
{
	GENERATED_BODY()

	/** JSON: "b_success" (output). True if discovery completed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	bool bSuccess = false;

	/** JSON: "blueprint_asset_path" (output). Context blueprint path, if provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString BlueprintAssetPath;

	/** JSON: "search_text" (output). Search term applied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	FString SearchText;

	/** JSON: "max_results" (output). Max results considered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	int32 MaxResults = 0;

	/** JSON: "descriptors" (output). Discovered node descriptors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	TArray<FSOTS_BPGenNodeSpawnerDescriptor> Descriptors;

	/** JSON: "errors" (output). Errors encountered during discovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	TArray<FString> Errors;

	/** JSON: "warnings" (output). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Discovery")
	TArray<FString> Warnings;
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
 * Summary of changes applied to a Blueprint asset.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenChangeSummary
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString BlueprintAssetPath;

	/** JSON: "target_type" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString TargetType;

	/** JSON: "target_name" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString TargetName;

	/** JSON: "created_node_ids" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> CreatedNodeIds;

	/** JSON: "updated_node_ids" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> UpdatedNodeIds;

	/** JSON: "deleted_node_ids" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> DeletedNodeIds;

	/** JSON: "created_vars" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> CreatedVariables;

	/** JSON: "updated_vars" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> UpdatedVariables;

	/** JSON: "created_functions" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> CreatedFunctions;

	/** JSON: "updated_functions" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> UpdatedFunctions;

	/** JSON: "created_widgets" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> CreatedWidgets;

	/** JSON: "updated_widgets" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> UpdatedWidgets;

	/** JSON: "property_sets" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> PropertySets;

	/** JSON: "bindings_created" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> BindingsCreated;
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

	/** JSON: "ErrorCode" (output only). Structured error code when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString ErrorCode;

	/** JSON: "error_codes" (output only, array). Non-fatal error codes during apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> ErrorCodes;

	/** JSON: "Warnings" (output only, array). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> Warnings;

	/** JSON: "auto_fix_steps" (output only, array). Auto-fix steps applied (if enabled). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FSOTS_BPGenAutoFixStep> AutoFixSteps;

	/** JSON: "repair_steps" (output only, array). Repair steps applied (if enabled). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result", meta = (JsonKey = "repair_steps"))
	TArray<FSOTS_BPGenRepairStep> RepairSteps;

	/** JSON: "spec_migrated" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result", meta = (JsonKey = "spec_migrated"))
	bool bSpecMigrated = false;

	/** JSON: "migration_notes" (output only, array). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result", meta = (JsonKey = "migration_notes"))
	TArray<FString> MigrationNotes;

	/** JSON: "CreatedNodeIds" (output only, array). NodeIds created during this apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> CreatedNodeIds;

	/** JSON: "UpdatedNodeIds" (output only, array). NodeIds updated during this apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> UpdatedNodeIds;

	/** JSON: "SkippedNodeIds" (output only, array). NodeIds skipped due to allow flags or missing references. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> SkippedNodeIds;

	/** JSON: "change_summary" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FSOTS_BPGenChangeSummary ChangeSummary;
};

/**
 * Function signature used by ensure primitives.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenFunctionSignature
{
	GENERATED_BODY()

	/** JSON: "Inputs" (optional). Function input params. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	TArray<FSOTS_BPGenPin> Inputs;

	/** JSON: "Outputs" (optional). Function outputs / return values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	TArray<FSOTS_BPGenPin> Outputs;

	/** JSON: "bPure" (optional). Whether function is pure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	bool bPure = false;

	/** JSON: "bConst" (optional). Whether function is const. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	bool bConst = false;
};

/** Result for ensure primitives. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenEnsureResult
{
	GENERATED_BODY()

	/** JSON: "b_success". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	bool bSuccess = false;

	/** JSON: "blueprint_path". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	FString BlueprintPath;

	/** JSON: "name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	FString Name;

	/** JSON: "ErrorMessage". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	FString ErrorMessage;

	/** JSON: "ErrorCode". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	FString ErrorCode;

	/** JSON: "Warnings". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	TArray<FString> Warnings;

	/** JSON: "change_summary" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Ensure")
	FSOTS_BPGenChangeSummary ChangeSummary;
};

/** Widget ensure request. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenWidgetSpec
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). Widget blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintAssetPath;

	/** JSON: "widget_class_path" (required). Widget class path (e.g., /Script/UMG.TextBlock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetClassPath;

	/** JSON: "widget_name" (required). Name for the widget in the tree. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "parent_name" (optional). Parent widget name; empty/root means set as RootWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ParentName;

	/** JSON: "insert_index" (optional). Insert index when attaching to a panel; -1 appends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	int32 InsertIndex = -1;

	/** JSON: "is_variable" (optional, defaults true). Whether to mark the widget as a variable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bIsVariable = true;

	/** JSON: "create_if_missing" (optional, defaults true). When false, missing widgets error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bCreateIfMissing = true;

	/** JSON: "update_if_exists" (optional, defaults true). When false, existing widgets are left untouched. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bUpdateIfExists = true;

	/** JSON: "reparent_if_mismatch" (optional, defaults true). Allow moving widget when parent differs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bReparentIfMismatch = true;

	/** JSON: "properties" (optional). Widget property map (PropertyPath -> value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TMap<FString, FString> Properties;

	/** JSON: "slot_properties" (optional). Slot property map (PropertyPath -> value) applied to Slot.*. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TMap<FString, FString> SlotProperties;
};

/** Widget ensure result. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenWidgetEnsureResult
{
	GENERATED_BODY()

	/** JSON: "b_success". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bSuccess = false;

	/** JSON: "blueprint_path". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintPath;

	/** JSON: "widget_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "parent_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ParentName;

	/** JSON: "slot_type" (output). Resolved slot type when attached to a panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString SlotType;

	/** JSON: "b_created" (output). True if widget was created. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bCreated = false;

	/** JSON: "b_reparented" (output). True if widget parent changed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bReparented = false;

	/** JSON: "applied_properties" (output). Property paths applied successfully. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TArray<FString> AppliedProperties;

	/** JSON: "warnings" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TArray<FString> Warnings;

	/** JSON: "error_message" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorMessage;

	/** JSON: "error_code" (output). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorCode;

	/** JSON: "change_summary" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FSOTS_BPGenChangeSummary ChangeSummary;
};

/** Widget property update request. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenWidgetPropertyRequest
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintAssetPath;

	/** JSON: "widget_name" (required). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "properties" (required). PropertyPath -> value map. Use Slot.* for slot properties. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TMap<FString, FString> Properties;

	/** JSON: "fail_if_missing" (optional). When true, missing widget errors out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bFailIfMissing = true;
};

/** Widget property update result. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenWidgetPropertyResult
{
	GENERATED_BODY()

	/** JSON: "b_success". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bSuccess = false;

	/** JSON: "blueprint_path". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintPath;

	/** JSON: "widget_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "applied_properties". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TArray<FString> AppliedProperties;

	/** JSON: "warnings". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TArray<FString> Warnings;

	/** JSON: "error_message". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorMessage;

	/** JSON: "error_code". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorCode;

	/** JSON: "change_summary" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FSOTS_BPGenChangeSummary ChangeSummary;
};

/** Binding ensure request for widget property bindings. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenBindingRequest
{
	GENERATED_BODY()

	/** JSON: "blueprint_asset_path" (required). Widget blueprint path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintAssetPath;

	/** JSON: "widget_name" (required). Widget to bind against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "property_name" (required). Property to bind (e.g., Text). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString PropertyName;

	/** JSON: "function_name" (required). Binding function name to create/associate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString FunctionName;

	/** JSON: "signature" (optional). Binding function signature (usually a return pin only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FSOTS_BPGenFunctionSignature Signature;

	/** JSON: "graph_spec" (optional). Graph spec to apply to the binding function. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FSOTS_BPGenGraphSpec GraphSpec;

	/** JSON: "apply_graph" (optional, defaults false). When true, apply graph_spec after ensure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bApplyGraph = false;

	/** JSON: "create_binding" (optional, defaults true). Allow creating binding entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bCreateBinding = true;

	/** JSON: "update_binding" (optional, defaults true). Allow updating existing binding entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bUpdateBinding = true;

	/** JSON: "create_function" (optional, defaults true). Allow creating the binding function. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bCreateFunction = true;

	/** JSON: "update_function" (optional, defaults true). Allow updating existing function signature. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bUpdateFunction = true;
};

/** Binding ensure result. */
USTRUCT(BlueprintType)
struct FSOTS_BPGenBindingEnsureResult
{
	GENERATED_BODY()

	/** JSON: "b_success". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bSuccess = false;

	/** JSON: "blueprint_path". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString BlueprintPath;

	/** JSON: "widget_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString WidgetName;

	/** JSON: "property_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString PropertyName;

	/** JSON: "function_name". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString FunctionName;

	/** JSON: "b_binding_created". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bBindingCreated = false;

	/** JSON: "b_binding_updated". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bBindingUpdated = false;

	/** JSON: "b_function_created". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bFunctionCreated = false;

	/** JSON: "b_function_updated". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	bool bFunctionUpdated = false;

	/** JSON: "warnings". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	TArray<FString> Warnings;

	/** JSON: "error_message". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorMessage;

	/** JSON: "error_code". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FString ErrorCode;

	/** JSON: "change_summary" (output only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|UMG")
	FSOTS_BPGenChangeSummary ChangeSummary;
};

/**
 * Summary information for an existing graph node (used by inspector APIs).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeSummary
{
	GENERATED_BODY()

	/** JSON: "node_id" (output). BPGen NodeId stored on the node (may be empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString NodeId;

	/** JSON: "node_guid" (output). Underlying node guid as string (always populated). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString NodeGuid;

	/** JSON: "node_class" (output). Node class short name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString NodeClass;

	/** JSON: "node_class_path" (output). Full node class path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString NodeClassPath;

	/** JSON: "title" (output). Display title of the node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString Title;

	/** JSON: "raw_name" (output). UObject name of the node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString RawName;

	/** JSON: "pos_x" (output). Node position X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	int32 NodePosX = 0;

	/** JSON: "pos_y" (output). Node position Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	int32 NodePosY = 0;

	/** JSON: "pins" (output). Optional pin descriptors when requested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FSOTS_BPGenDiscoveredPinDescriptor> Pins;
};

/**
 * Pin default value captured during node describe.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenPinDefault
{
	GENERATED_BODY()

	/** JSON: "pin_name" (output). Pin name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FName PinName;

	/** JSON: "default_value" (output). Default value literal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString DefaultValue;
};

/**
 * Link description captured during node describe.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeLink
{
	GENERATED_BODY()

	/** JSON: "from_node_id" (output). Source node id (NodeId or NodeGuid fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString FromNodeId;

	/** JSON: "from_pin" (output). Source pin name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FName FromPinName;

	/** JSON: "to_node_id" (output). Target node id (NodeId or NodeGuid fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString ToNodeId;

	/** JSON: "to_pin" (output). Target pin name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FName ToPinName;
};

/**
 * Detailed node description (pins, defaults, links).
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeDescription
{
	GENERATED_BODY()

	/** JSON: "summary" (output). Basic node summary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FSOTS_BPGenNodeSummary Summary;

	/** JSON: "links" (output). Links originating from this node's pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FSOTS_BPGenNodeLink> Links;

	/** JSON: "pin_defaults" (output). Default values for input pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FSOTS_BPGenPinDefault> PinDefaults;
};

/**
 * Result for ListFunctionGraphNodes.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeListResult
{
	GENERATED_BODY()

	/** JSON: "b_success" (output). True if list succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	bool bSuccess = false;

	/** JSON: "blueprint_path" (output). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString BlueprintPath;

	/** JSON: "function_name" (output). Function graph inspected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FName FunctionName;

	/** JSON: "nodes" (output). Node summaries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FSOTS_BPGenNodeSummary> Nodes;

	/** JSON: "errors" (output). Fatal errors encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FString> Errors;

	/** JSON: "warnings" (output). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FString> Warnings;
};

/**
 * Result for DescribeNodeById.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenNodeDescribeResult
{
	GENERATED_BODY()

	/** JSON: "b_success" (output). True if describe succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	bool bSuccess = false;

	/** JSON: "blueprint_path" (output). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString BlueprintPath;

	/** JSON: "function_name" (output). Function graph inspected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FName FunctionName;

	/** JSON: "node_id" (output). NodeId requested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FString NodeId;

	/** JSON: "description" (output). Full node description. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	FSOTS_BPGenNodeDescription Description;

	/** JSON: "errors" (output). Fatal errors encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FString> Errors;

	/** JSON: "warnings" (output). Non-fatal warnings encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Inspect")
	TArray<FString> Warnings;
};

/**
 * Maintenance result for compile/save/refresh helpers.
 */
USTRUCT(BlueprintType)
struct FSOTS_BPGenMaintenanceResult
{
	GENERATED_BODY()

	/** JSON: "b_success" (output). True if operation succeeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	bool bSuccess = false;

	/** JSON: "blueprint_path" (output). Blueprint asset path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString BlueprintPath;

	/** JSON: "function_name" (output). Optional function graph context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FName FunctionName;

	/** JSON: "message" (output). Informational message. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	FString Message;

	/** JSON: "nodes" (output). Optional node summaries when provided by maintenance helpers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FSOTS_BPGenNodeSummary> Nodes;

	/** JSON: "errors" (output). Fatal errors encountered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BPGen|Result")
	TArray<FString> Errors;

	/** JSON: "warnings" (output). Non-fatal warnings encountered. */
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
