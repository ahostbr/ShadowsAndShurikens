#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_ShaderWarmupTypes.h"
#include "Containers/Ticker.h"
#include "Templates/SharedPointer.h"
#include "SOTS_ShaderWarmupSubsystem.generated.h"

class AActor;
class FStreamableHandle;
class UNiagaraComponent;
class UStaticMeshComponent;
class UWorld;

UCLASS()
class SOTS_UI_API USOTS_ShaderWarmupSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USOTS_ShaderWarmupSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Warmup")
	bool BeginWarmup(const F_SOTS_ShaderWarmupRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Warmup")
	void EndWarmup(bool bRestoreTimeDilation = true);

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Warmup")
	FSOTS_OnShaderWarmupProgress OnProgress;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Warmup")
	FSOTS_OnShaderWarmupFinished OnFinished;

private:
	void BuildAssetQueue(const F_SOTS_ShaderWarmupRequest& Request);
	void LoadNext();
	void HandleAssetLoaded(FSoftObjectPath LoadedPath);
	void BeginWaitForCompiling();
	bool HandleCompilePoll(float DeltaTime);
	bool IsStillCompiling() const;
	void EnsureTouchActor();
	void EnsureTouchMesh();
	void EnsureTouchNiagara();

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void SetupMoviePlayerScreen();
	FGameplayTag ResolveWidgetId(const F_SOTS_ShaderWarmupRequest& Request) const;

	UPROPERTY()
	bool bWarmupActive = false;

	UPROPERTY()
	bool bMoviePlayerHooksBound = false;

	UPROPERTY()
	bool bAppliedTimeDilation = false;

	UPROPERTY()
	F_SOTS_ShaderWarmupRequest ActiveRequest;

	UPROPERTY()
	TArray<FSoftObjectPath> AssetQueue;

	UPROPERTY()
	int32 TotalCount = 0;

	UPROPERTY()
	int32 DoneCount = 0;

	UPROPERTY()
	int32 QueueIndex = 0;

	UPROPERTY()
	float PrevTimeDilation = 1.0f;

	UPROPERTY()
	TWeakObjectPtr<AActor> TouchActor;

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> TouchMesh;

	UPROPERTY()
	TWeakObjectPtr<UNiagaraComponent> TouchNiagara;

	TSharedPtr<FStreamableHandle> ActiveLoadHandle;
	FTSTicker::FDelegateHandle CompilePollHandle;
};
