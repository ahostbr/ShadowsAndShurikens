// Copyright Yuzhda Bohdan (Bitkovin)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShaderCompilationScreenBPLibrary.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMaterialCompilationFinishedDelegate);

UCLASS()
class UShaderCompilationScreenBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable,Category = "Shader Compilation")
	static TArray<TSoftObjectPtr<UMaterial>> GetAllMaterials();
	
	UFUNCTION(BlueprintCallable,Category = "Shader Compilation")
	static TArray<TSoftObjectPtr<UNiagaraSystem>> GetAllNiagaraParticleSystems();

	UFUNCTION(BlueprintCallable,Category = "Shader Compilation")
	static bool AreShadersCompiling();
};
