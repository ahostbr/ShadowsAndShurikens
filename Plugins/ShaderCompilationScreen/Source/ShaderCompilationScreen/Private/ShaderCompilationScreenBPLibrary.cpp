// Copyright Yuzhda Bohdan (Bitkovin) 2023

#include "ShaderCompilationScreenBPLibrary.h"
#include "ShaderCompilationScreen.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Materials/Material.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ShaderCompiler.h"

UShaderCompilationScreenBPLibrary::UShaderCompilationScreenBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

TArray<TSoftObjectPtr<UMaterial>> UShaderCompilationScreenBPLibrary::GetAllMaterials()
{
	TArray<TSoftObjectPtr<UMaterial>> MaterialReferences;

	// Get the asset registry module
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Create a filter to retrieve only materials
	FARFilter Filter;
	Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());

	// Retrieve all asset data that matches the filter
	TArray<FAssetData> AssetDataArray;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataArray);

	// Store the soft object references of the materials
	for (const FAssetData& AssetData : AssetDataArray)
	{
		TSoftObjectPtr<UMaterial> MaterialReference(AssetData.ToSoftObjectPath());
		MaterialReferences.AddUnique(MaterialReference);
	}

	return MaterialReferences;
}

TArray<TSoftObjectPtr<UNiagaraSystem>> UShaderCompilationScreenBPLibrary::GetAllNiagaraParticleSystems()
{
	TArray<TSoftObjectPtr<UNiagaraSystem>> ParticleSystemReferences;

	// Get the asset registry module
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Create a filter to retrieve only Niagara particle systems
	FARFilter Filter;
	Filter.ClassPaths.Add(UNiagaraSystem::StaticClass()->GetClassPathName());

	// Retrieve all asset data that matches the filter
	TArray<FAssetData> AssetDataArray;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataArray);

	// Store the soft object references of the Niagara particle systems
	for (const FAssetData& AssetData : AssetDataArray)
	{
		TSoftObjectPtr<UNiagaraSystem> ParticleSystemReference(AssetData.ToSoftObjectPath());
		ParticleSystemReferences.AddUnique(ParticleSystemReference);
	}

	return ParticleSystemReferences;
}

bool UShaderCompilationScreenBPLibrary::AreShadersCompiling()
{
#if !UE_EDITOR
	return false;
#else
	return (GShaderCompilingManager->GetNumPendingJobs() + GShaderCompilingManager->GetNumRemainingJobs()) > 0;
#endif
}
