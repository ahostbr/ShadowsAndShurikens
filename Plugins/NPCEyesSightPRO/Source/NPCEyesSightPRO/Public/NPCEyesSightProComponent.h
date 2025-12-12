// Copyright 2021-2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Engine.h"
#include "NPCEyesSightProComponent.generated.h"

USTRUCT(BlueprintType)
struct FNPCEyesLightParameters
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	float attenuationRadius = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	bool bIsVisible = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	bool bIsCastShadow = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	float intensity = 0.f;

	FNPCEyesLightParameters()
	{
	};
};

USTRUCT(BlueprintType)
struct FSeeEnemiesDataPro
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	AActor *seeActor = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	FName socketName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	FHitResult returnHitRes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Out")
	bool bIsShadow = false;

	FSeeEnemiesDataPro()
	{
	};
};

USTRUCT(BlueprintType)
struct FNPCEyesParametersPro
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	TArray<TSubclassOf<AActor>> enemyActors;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	float maxViewingAngle = 90.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	float maxViewingDistance = 5000.f;
	// NPC can see shadow from target.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	bool bIsCanFindShadow = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	float sunDistance = 20000.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	float sunShadowDistance = 2000.f;
	// Sees only if the target is illuminated
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	bool bIsTargetShouldBeIlluminated = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	bool bIsUseComplexTrace = true;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "NPC Eyes Parameters")
	TEnumAsByte<ECollisionChannel> collisionChannel = ECollisionChannel::ECC_Visibility;

	FNPCEyesParametersPro()
	{
	};
};

UCLASS(Blueprintable)
class NPCEYESSIGHTPRO_API UNPCEyesSightProComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSeeEnemiesDelegate, TArray<FSeeEnemiesDataPro>, OutEnemiesData, TArray<FHitResult>, HitResultArr);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotSeeEnemiesDelegate);

public:	
	// Sets default values for this component's properties
	UNPCEyesSightProComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
    void SetNPCEyesComponent_BP(class UArrowComponent* setArrow);

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
    void DisableNPCEyes_BP();

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	void EnableNPCEyes_BP();

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	void UpdateNpcEyesParameters_BP(FNPCEyesParametersPro updateNpcEyesParameters);

	UFUNCTION(BlueprintPure, Category = "NPC Eyes PRO")
	FNPCEyesParametersPro GetNpcEyesParameters_BP();

	void StartWork();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	FNPCEyesParametersPro npcEyesParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters", meta=(ClampMin="0.1", ClampMax="100"))
	float rateSeeEvents = 0.1f;

	// If you want to calculate vision on the client side as well, enable this option. If you want to calculate vision only on the server side, leave this option disabled.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	bool bEnableCalculationsOnClient = false;
	
	// Broadcasts notification that our sensor sees a Actors, using the FSeeEnemiesDelegate delegates.
	void BroadcastOnISeeEnemiesActors(TArray<FSeeEnemiesDataPro> OutEnemiesData, TArray<FHitResult> HitResultArr) const;

	// Broadcasts notification that our sensor not sees a Actors, using the FNotSeeEnemiesDelegate delegates.
	void BroadcastOnINotSeeEnemiesActors() const;

	/** Delegate to execute when we see a Pawn. */
	UPROPERTY(BlueprintAssignable)
	FSeeEnemiesDelegate OnSeeEnemiesActors;

	/** Delegate to execute when we not see a Pawn. */
	UPROPERTY(BlueprintAssignable)
	FNotSeeEnemiesDelegate OnNotSeeEnemiesActors;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginDestroy() override;

private:

	class NPCEyesProThread* npcEyesSightThread = nullptr;

	UPROPERTY()
	class UArrowComponent *npcEyesComponentRef;
	
	FTimerHandle getEnemiesActor_Timer;

	FTimerHandle getSeeEnemiesActor_Timer;

	void InitNPCEyesDelay() const;
	void GetSeeEnemiesActorTimer();
	
};

// Thread
class NPCEyesProThread : public FRunnable
{
public:

	//================================= THREAD =====================================
    
	//Constructor
	NPCEyesProThread(AActor *newActor);
	//Destructor
	virtual ~NPCEyesProThread() override;

	//Use this method to kill the thread!!
	void EnsureCompletion();
	//Pause the thread 
	void PauseThread();
	//Continue/UnPause the thread
	void ContinueThread();
	//FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	bool IsThreadPaused() const;    

	void SetEyesComponent(class UArrowComponent *setNpcEyesComponent);
	void SetEyesParameters(FNPCEyesParametersPro setEyesParameters);

	AActor *ownerActor;

	TArray<FSeeEnemiesDataPro> GetSeeEnemiesData(TArray<AActor*> setEnemiesActors, FNPCEyesParametersPro setEyesParameters);
    

private:

	//Thread to run the worker FRunnable on
	FRunnableThread* Thread;

	FCriticalSection m_mutex;
	FEvent* m_semaphore;

	int m_chunkCount;
	int m_amount;
	int m_MinInt;
	int m_MaxInt;

	//As the name states those members are Thread safe
	FThreadSafeBool m_Kill;
	FThreadSafeBool m_Pause;

	FNPCEyesParametersPro npcEyesParametersTD;

	class UArrowComponent *npcEyesComponentRefTD = nullptr;

	TArray<FSeeEnemiesDataPro> seeEnemiesDataArr;	

	FHitResult CheckLineTraceTD(FVector startV, FVector &endV, bool setTraceComplex, AActor* setIgnoreActor) const;

	TArray<AActor*> enemiesActorsArrTD;

	float rateSeeEventsInThread = 0.1f;

};
