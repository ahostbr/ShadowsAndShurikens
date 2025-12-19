#include "SOTS_BPGenDescriptorTranslator.h"

FSOTS_BPGenGraphNode USOTS_BPGenDescriptorTranslator::MakeGraphNodeFromDescriptor(const FSOTS_BPGenNodeSpawnerDescriptor& Descriptor, const FString& NodeIdOverride)
{
	FSOTS_BPGenGraphNode Node;
	Node.Id = !NodeIdOverride.IsEmpty() ? NodeIdOverride : (!Descriptor.SpawnerKey.IsEmpty() ? Descriptor.SpawnerKey : Descriptor.DisplayName);
	if (Node.Id.IsEmpty())
	{
		Node.Id = TEXT("Node");
	}

	Node.SpawnerKey = Descriptor.SpawnerKey;
	Node.bPreferSpawnerKey = true;

	if (!Descriptor.NodeClassName.IsEmpty())
	{
		Node.NodeType = FName(*Descriptor.NodeClassName);
	}
	else if (!Descriptor.NodeType.IsEmpty())
	{
		Node.NodeType = FName(*Descriptor.NodeType);
	}

	Node.FunctionPath = Descriptor.FunctionPath;
	Node.StructPath = Descriptor.StructPath;
	Node.VariableName = Descriptor.VariableName;

	// Provide a few optional extras for variable/cast spawners.
	if (!Descriptor.VariableOwnerClassPath.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("VariableOwnerClassPath")), Descriptor.VariableOwnerClassPath);
	}
	if (!Descriptor.TargetClassPath.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("TargetClassPath")), Descriptor.TargetClassPath);
	}

	// Pin hints (useful for custom handling later).
	if (!Descriptor.VariablePinCategory.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("PinCategory")), Descriptor.VariablePinCategory);
	}
	if (!Descriptor.VariablePinSubCategory.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("PinSubCategory")), Descriptor.VariablePinSubCategory);
	}
	if (!Descriptor.VariablePinSubObjectPath.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("SubObjectPath")), Descriptor.VariablePinSubObjectPath);
	}
	if (!Descriptor.VariablePinContainerType.IsEmpty())
	{
		Node.ExtraData.Add(FName(TEXT("ContainerType")), Descriptor.VariablePinContainerType);
	}

	return Node;
}

FSOTS_BPGenGraphLink USOTS_BPGenDescriptorTranslator::MakeLinkSpec(const FString& FromNodeId, FName FromPinName, const FString& ToNodeId, FName ToPinName, bool bUseSchema)
{
	FSOTS_BPGenGraphLink Link;
	Link.FromNodeId = FromNodeId;
	Link.FromPinName = FromPinName;
	Link.ToNodeId = ToNodeId;
	Link.ToPinName = ToPinName;
	Link.bUseSchema = bUseSchema;
	return Link;
}
