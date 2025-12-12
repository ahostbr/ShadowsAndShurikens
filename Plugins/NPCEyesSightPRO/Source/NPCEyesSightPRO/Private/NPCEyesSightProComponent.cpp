// Copyright 2021-2024 CAS. All Rights Reserved.


#include "NPCEyesSightProComponent.h"

#include "NPCEyesPointsPro.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/ArrowComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
UNPCEyesSightProComponent::UNPCEyesSightProComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UNPCEyesSightProComponent::BroadcastOnISeeEnemiesActors(TArray<FSeeEnemiesDataPro> OutEnemiesData, TArray<FHitResult> HitResultArr) const
{
	OnSeeEnemiesActors.Broadcast(OutEnemiesData, HitResultArr);
}

void UNPCEyesSightProComponent::BroadcastOnINotSeeEnemiesActors() const
{
	OnNotSeeEnemiesActors.Broadcast();
}

// Called when the game starts
void UNPCEyesSightProComponent::BeginPlay()
{
	Super::BeginPlay();

	StartWork();
}

void UNPCEyesSightProComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (npcEyesSightThread)
	{
		npcEyesSightThread->EnsureCompletion();
		delete npcEyesSightThread;
		npcEyesSightThread = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UNPCEyesSightProComponent::BeginDestroy()
{
	if (npcEyesSightThread)
	{
		npcEyesSightThread->EnsureCompletion();
		delete npcEyesSightThread;
		npcEyesSightThread = nullptr;
	}

	Super::BeginDestroy();
}

// Called every frame
void UNPCEyesSightProComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UNPCEyesSightProComponent::InitNPCEyesDelay() const
{
	if (npcEyesSightThread)
	{
		npcEyesSightThread->SetEyesComponent(npcEyesComponentRef);
		npcEyesSightThread->SetEyesParameters(npcEyesParameters);
	}
}

void UNPCEyesSightProComponent::GetSeeEnemiesActorTimer()
{
	// Find all enemies.
	TArray<AActor*> allEnemy_;
	for (int i = 0; i < npcEyesParameters.enemyActors.Num(); i++)
	{
		TArray<AActor*> getEnemy_;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), npcEyesParameters.enemyActors[i], getEnemy_);

		for (int id = 0; id < getEnemy_.Num(); id++)
		{
			allEnemy_.AddUnique(getEnemy_[id]);
		}
	}

	if (npcEyesSightThread)
	{
		TArray<FHitResult> returnHitResArr_;
		TArray<FSeeEnemiesDataPro> allEnemiesDataArr_ = npcEyesSightThread->GetSeeEnemiesData(allEnemy_, npcEyesParameters);

		for (int i = 0; i < allEnemiesDataArr_.Num(); i++)
		{
			FSeeEnemiesDataPro newEnemyData_ = allEnemiesDataArr_[i];
			returnHitResArr_.Add(newEnemyData_.returnHitRes);
		}

		if (returnHitResArr_.Num() > 0)
		{
			BroadcastOnISeeEnemiesActors(allEnemiesDataArr_, returnHitResArr_);
		}
		else
		{
			BroadcastOnINotSeeEnemiesActors();
		}
	}
}

void UNPCEyesSightProComponent::SetNPCEyesComponent_BP(UArrowComponent* setArrow)
{
	npcEyesComponentRef = setArrow;
}

void UNPCEyesSightProComponent::DisableNPCEyes_BP()
{
	if (npcEyesSightThread)
	{
		GetWorld()->GetTimerManager().ClearTimer(getSeeEnemiesActor_Timer);
		npcEyesSightThread->EnsureCompletion();
		delete npcEyesSightThread;
		npcEyesSightThread = nullptr;
	}
}

void UNPCEyesSightProComponent::EnableNPCEyes_BP()
{
	if (!npcEyesSightThread)
	{
		if (!bEnableCalculationsOnClient)
		{
			if (GetOwner()->HasAuthority())
			{
				npcEyesSightThread = nullptr;
				npcEyesSightThread = new NPCEyesProThread(GetOwner());

				// FTimerHandle delayInit_Timer_;
				// GetWorld()->GetTimerManager().SetTimer(delayInit_Timer_, this, &UNPCEyesSightProComponent::InitNPCEyesDelay, 1.f, false, 0.5f);
				InitNPCEyesDelay();
				GetWorld()->GetTimerManager().SetTimer(getSeeEnemiesActor_Timer, this, &UNPCEyesSightProComponent::GetSeeEnemiesActorTimer, rateSeeEvents, true, rateSeeEvents);
			}
		}
		else
		{
			npcEyesSightThread = nullptr;
			npcEyesSightThread = new NPCEyesProThread(GetOwner());

			// FTimerHandle delayInit_Timer_;
			// GetWorld()->GetTimerManager().SetTimer(delayInit_Timer_, this, &UNPCEyesSightProComponent::InitNPCEyesDelay, 1.f, false, 0.5f);
			InitNPCEyesDelay();
			GetWorld()->GetTimerManager().SetTimer(getSeeEnemiesActor_Timer, this, &UNPCEyesSightProComponent::GetSeeEnemiesActorTimer, rateSeeEvents, true, rateSeeEvents);
		}
	}
}

void UNPCEyesSightProComponent::UpdateNpcEyesParameters_BP(FNPCEyesParametersPro updateNpcEyesParameters)
{
	npcEyesParameters = updateNpcEyesParameters;
}

FNPCEyesParametersPro UNPCEyesSightProComponent::GetNpcEyesParameters_BP()
{
	return npcEyesParameters;
}


void UNPCEyesSightProComponent::StartWork()
{
	if (!npcEyesSightThread)
	{
		if (!bEnableCalculationsOnClient)
		{
			if (GetOwner()->HasAuthority())
			{
				npcEyesSightThread = nullptr;
				npcEyesSightThread = new NPCEyesProThread(GetOwner());

				FTimerHandle delayInit_Timer_;
				GetWorld()->GetTimerManager().SetTimer(delayInit_Timer_, this, &UNPCEyesSightProComponent::InitNPCEyesDelay, 1.f, false, 0.5f);
				GetWorld()->GetTimerManager().SetTimer(getSeeEnemiesActor_Timer, this, &UNPCEyesSightProComponent::GetSeeEnemiesActorTimer, rateSeeEvents, true, rateSeeEvents);
			}
		}
		else
		{
			npcEyesSightThread = nullptr;
			npcEyesSightThread = new NPCEyesProThread(GetOwner());

			FTimerHandle delayInit_Timer_;
			GetWorld()->GetTimerManager().SetTimer(delayInit_Timer_, this, &UNPCEyesSightProComponent::InitNPCEyesDelay, 1.f, false, 0.5f);
			GetWorld()->GetTimerManager().SetTimer(getSeeEnemiesActor_Timer, this, &UNPCEyesSightProComponent::GetSeeEnemiesActorTimer, rateSeeEvents, true, rateSeeEvents);
		}
	}
}

NPCEyesProThread::NPCEyesProThread(AActor* newActor)
{
	m_Kill = false;
	m_Pause = false;

	//Initialize FEvent (as a cross platform (Confirmed Mac/Windows))
	m_semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);
	ownerActor = newActor;

	Thread = FRunnableThread::Create(this, (TEXT("%s_FSomeRunnable"), *ownerActor->GetName()), 0, TPri_Lowest);
}

NPCEyesProThread::~NPCEyesProThread()
{
	if (m_semaphore)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(m_semaphore);
		m_semaphore = nullptr;
	}

	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
}

void NPCEyesProThread::EnsureCompletion()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void NPCEyesProThread::PauseThread()
{
	m_Pause = true;
}

void NPCEyesProThread::ContinueThread()
{
	m_Pause = false;

	if (m_semaphore)
	{
		//Here is a FEvent signal "Trigger()" -> it will wake up the thread.
		m_semaphore->Trigger();
	}
}

bool NPCEyesProThread::Init()
{
	return true;
}

uint32 NPCEyesProThread::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(FMath::RandRange(0.05f, 0.5f));

	while (!m_Kill)
	{
		if (m_Pause)
		{
			//FEvent->Wait(); will "sleep" the thread until it will get a signal "Trigger()"
			m_semaphore->Wait();

			if (m_Kill)
			{
				return 0;
			}
		}
		else
		{
			m_mutex.Lock();

			TArray<AActor*> allActorsArr_ = enemiesActorsArrTD;

			m_mutex.Unlock();

			TArray<FSeeEnemiesDataPro> seeDataArr_;
			for (int enemyID_ = 0; enemyID_ < allActorsArr_.Num(); enemyID_++)
			{
				if (!npcEyesComponentRefTD)
				{
					break;
				}

				UNPCEyesPointsPro* eyesPointsRef_;
				eyesPointsRef_ = Cast<UNPCEyesPointsPro>(allActorsArr_[enemyID_]->GetComponentByClass(UNPCEyesPointsPro::StaticClass()));
				if (eyesPointsRef_)
				{
					TArray<FName> pointsName_ = eyesPointsRef_->GetPointsName();

					TArray<USceneComponent*> allLightsArr_ = eyesPointsRef_->GetLights();

					// If can see shadows.
					if (npcEyesParametersTD.bIsCanFindShadow)
					{
						bool bIsSeeTargetShadow_(false);
						// Check all lights.
						for (int lightsID_ = 0; lightsID_ < allLightsArr_.Num(); ++lightsID_)
						{
							UDirectionalLightComponent* directLight_ = Cast<UDirectionalLightComponent>(allLightsArr_[lightsID_]);
							UPointLightComponent* pointLight_ = Cast<UPointLightComponent>(allLightsArr_[lightsID_]);
							if (pointLight_ || directLight_)
							{
								FNPCEyesLightParameters lightParameters_;
								FVector sunForwardV_;

								if (directLight_)
								{
									sunForwardV_ = UKismetMathLibrary::GetForwardVector(directLight_->GetComponentRotation());
									lightParameters_.attenuationRadius = npcEyesParametersTD.sunDistance;
									lightParameters_.bIsVisible = directLight_->IsVisible();
									lightParameters_.intensity = directLight_->Intensity;
									lightParameters_.bIsCastShadow = directLight_->CastShadows;
								}
								else
								{
									lightParameters_.attenuationRadius = pointLight_->AttenuationRadius;
									lightParameters_.bIsVisible = pointLight_->IsVisible();
									lightParameters_.intensity = pointLight_->Intensity;
									lightParameters_.bIsCastShadow = pointLight_->CastShadows;
								}

								if (lightParameters_.bIsVisible && lightParameters_.intensity > 0.f && lightParameters_.bIsCastShadow)
								{
									// Check all sockets.
									for (int socketID_ = 0; socketID_ < pointsName_.Num(); socketID_++)
									{
										if (eyesPointsRef_->GetComponentForSight())
										{
											FVector socketLocation_ = eyesPointsRef_->GetComponentForSight()->GetSocketLocation(pointsName_[socketID_]);
											FVector lightLocation_;

											if (directLight_)
											{
												lightLocation_ = sunForwardV_ * -(lightParameters_.attenuationRadius - 100.f) + socketLocation_;
											}
											else
											{
												lightLocation_ = pointLight_->GetComponentLocation();
											}

											FHitResult checkLightToTargetHitResult_ = CheckLineTraceTD(lightLocation_, socketLocation_, npcEyesParametersTD.bIsUseComplexTrace, ownerActor);

											// If the light illuminates a part of the body.
											if (checkLightToTargetHitResult_.GetActor() == allActorsArr_[enemyID_] &&
												(socketLocation_ - lightLocation_).Size() < lightParameters_.attenuationRadius)
											{
												FRotator lightLineRotation_ = UKismetMathLibrary::FindLookAtRotation(lightLocation_, socketLocation_);
												FVector shadowLocation_;
												if (directLight_)
												{
													shadowLocation_ = lightLineRotation_.Vector() * (lightParameters_.attenuationRadius + npcEyesParametersTD.sunShadowDistance) + lightLocation_;
												}
												else
												{
													shadowLocation_ = lightLineRotation_.Vector() * lightParameters_.attenuationRadius + lightLocation_;
												}

												FHitResult checkShadowHitResult_;

												USpotLightComponent* spotLight_ = Cast<USpotLightComponent>(allLightsArr_[lightsID_]);
												if (spotLight_)
												{
													FRotator rotTarget_ = UKismetMathLibrary::FindLookAtRotation(lightLocation_, socketLocation_);
													float lightAngle_ = FMath::RadiansToDegrees(acosf(FVector::DotProduct(pointLight_->GetForwardVector(), rotTarget_.Vector())));

													if (lightAngle_ <= spotLight_->OuterConeAngle)
													{
														checkShadowHitResult_ = CheckLineTraceTD(socketLocation_, shadowLocation_, npcEyesParametersTD.bIsUseComplexTrace, allActorsArr_[enemyID_]);
													}
												}
												else
												{
													checkShadowHitResult_ = CheckLineTraceTD(socketLocation_, shadowLocation_, npcEyesParametersTD.bIsUseComplexTrace, allActorsArr_[enemyID_]);
												}

												if (checkShadowHitResult_.bBlockingHit && checkShadowHitResult_.Location != FVector::ZeroVector)
												{
													FVector shadowHit_ = checkShadowHitResult_.Location;

													// Find view angle.
													FRotator rotTarget_ = UKismetMathLibrary::FindLookAtRotation(npcEyesComponentRefTD->GetComponentLocation(), shadowHit_);
													float const viewAngle_ = FMath::RadiansToDegrees(acosf(FVector::DotProduct(npcEyesComponentRefTD->GetForwardVector(), rotTarget_.Vector())));

													if ((npcEyesComponentRefTD->GetComponentLocation() - shadowHit_).Size() <= npcEyesParametersTD.maxViewingDistance &&
														viewAngle_ <= npcEyesParametersTD.maxViewingAngle)
													{
														FVector eyesLoc_ = npcEyesComponentRefTD->GetComponentLocation();
														FHitResult checkHitResult_ = CheckLineTraceTD(eyesLoc_, shadowHit_, true, ownerActor);


														if (checkHitResult_.Location.Equals(shadowHit_, 1.f)) //(checkHitResult_.GetActor() == ownerActor)
														{
															FSeeEnemiesDataPro newEnemy_;
															newEnemy_.seeActor = allActorsArr_[enemyID_];
															newEnemy_.returnHitRes = checkHitResult_;
															newEnemy_.socketName = pointsName_[socketID_];
															newEnemy_.bIsShadow = true;

															seeDataArr_.Add(newEnemy_);
															bIsSeeTargetShadow_ = true;
														}

														if (bIsSeeTargetShadow_)
														{
															break;
														}
													}
												}
											}
										}
									}
								}
							}
							// Break lights cycle.
							if (bIsSeeTargetShadow_)
							{
								break;
							}
						}
					}

					// Check all sockets.
					for (int socketID_ = 0; socketID_ < pointsName_.Num(); socketID_++)
					{
						if (eyesPointsRef_->GetComponentForSight())
						{
							FVector socketLocation_ = eyesPointsRef_->GetComponentForSight()->GetSocketLocation(pointsName_[socketID_]);

							bool bIsCanCheckSocket_(true);

							if (npcEyesParametersTD.bIsTargetShouldBeIlluminated)
							{
								bool bIsSocketIsLight_(false);

								for (int lightID_ = 0; lightID_ < allLightsArr_.Num(); ++lightID_)
								{
									UDirectionalLightComponent* directLight_ = Cast<UDirectionalLightComponent>(allLightsArr_[lightID_]);
									UPointLightComponent* pointLight_ = Cast<UPointLightComponent>(allLightsArr_[lightID_]);
									if (pointLight_ || directLight_)
									{
										FNPCEyesLightParameters lightParameters_;
										FVector lightLocation_;

										if (directLight_)
										{
											lightParameters_.attenuationRadius = npcEyesParametersTD.sunDistance;
											lightParameters_.bIsVisible = directLight_->IsVisible();
											lightParameters_.intensity = directLight_->Intensity;
											lightParameters_.bIsCastShadow = directLight_->CastShadows;
											lightLocation_ = UKismetMathLibrary::GetForwardVector(directLight_->GetComponentRotation()) * -(lightParameters_.attenuationRadius - 100.f) + socketLocation_;
										}
										else
										{
											lightParameters_.attenuationRadius = pointLight_->AttenuationRadius;
											lightParameters_.bIsVisible = pointLight_->IsVisible();
											lightParameters_.intensity = pointLight_->Intensity;
											lightParameters_.bIsCastShadow = pointLight_->CastShadows;
											lightLocation_ = pointLight_->GetComponentLocation();
										}


										if (lightParameters_.bIsVisible && lightParameters_.intensity > 0.f &&
											lightParameters_.attenuationRadius >= (lightLocation_ - socketLocation_).Size())
										{
											FHitResult checkLightSocketHitResult_;

											USpotLightComponent* spotLight_ = Cast<USpotLightComponent>(allLightsArr_[lightID_]);
											if (spotLight_)
											{
												FRotator rotTarget_ = UKismetMathLibrary::FindLookAtRotation(lightLocation_, socketLocation_);
												float lightAngle_ = FMath::RadiansToDegrees(acosf(FVector::DotProduct(pointLight_->GetForwardVector(), rotTarget_.Vector())));

												if (lightAngle_ <= spotLight_->OuterConeAngle)
												{
													checkLightSocketHitResult_ = CheckLineTraceTD(lightLocation_, socketLocation_, npcEyesParametersTD.bIsUseComplexTrace, ownerActor);
												}
											}
											else
											{
												checkLightSocketHitResult_ = CheckLineTraceTD(lightLocation_, socketLocation_, npcEyesParametersTD.bIsUseComplexTrace, ownerActor);
											}

											if (checkLightSocketHitResult_.bBlockingHit && checkLightSocketHitResult_.Location != FVector::ZeroVector)
											{
												if (checkLightSocketHitResult_.GetActor() == allActorsArr_[enemyID_])
												{
													bIsSocketIsLight_ = true;
												}
											}
										}
									}
								}

								bIsCanCheckSocket_ = bIsSocketIsLight_;
							}


							// Find view angle.
							FRotator rotTarget_ = UKismetMathLibrary::FindLookAtRotation(npcEyesComponentRefTD->GetComponentLocation(), socketLocation_);
							float const viewAngle_ = FMath::RadiansToDegrees(acosf(FVector::DotProduct(npcEyesComponentRefTD->GetForwardVector(), rotTarget_.Vector())));

							if (bIsCanCheckSocket_)
							{
								if ((npcEyesComponentRefTD->GetComponentLocation() - socketLocation_).Size() <= npcEyesParametersTD.maxViewingDistance &&
									viewAngle_ <= npcEyesParametersTD.maxViewingAngle)
								{
									bool bIsSee_(false);

									if (npcEyesComponentRefTD)
									{
										FHitResult checkHitResult_ = CheckLineTraceTD(npcEyesComponentRefTD->GetComponentLocation(), socketLocation_, npcEyesParametersTD.bIsUseComplexTrace, ownerActor);

										if (checkHitResult_.GetActor() == allActorsArr_[enemyID_])
										{
											FSeeEnemiesDataPro newEnemy_;
											newEnemy_.seeActor = checkHitResult_.GetActor();
											newEnemy_.returnHitRes = checkHitResult_;
											newEnemy_.socketName = pointsName_[socketID_];

											seeDataArr_.Add(newEnemy_);
											bIsSee_ = true;
										}
									}
									if (bIsSee_)
									{
										break;
									}
								}
							}
						}
					}
				}
			}

			//Critical section:
			m_mutex.Lock();
			//We are locking our FCriticalSection so no other thread will access it
			//And thus it is a thread-safe access now

			seeEnemiesDataArr = seeDataArr_;

			//Unlock FCriticalSection so other threads may use it.
			m_mutex.Unlock();

			//Pause Condition
			m_Pause = true;


			//A little sleep between the chunks (So CPU will rest a bit -- (may be omitted))
			FPlatformProcess::Sleep(rateSeeEventsInThread);
		}
	}
	return 0;
}

void NPCEyesProThread::Stop()
{
	m_Kill = true; //Thread kill condition "while (!m_Kill){...}"
	m_Pause = false;

	if (m_semaphore)
	{
		//We shall signal "Trigger" the FEvent (in case the Thread is sleeping it shall wake up!!)
		m_semaphore->Trigger();
	}
}

bool NPCEyesProThread::IsThreadPaused() const
{
	return (bool)m_Pause;
}

void NPCEyesProThread::SetEyesComponent(UArrowComponent* setNpcEyesComponent)
{
	if (IsValid(setNpcEyesComponent))
	{
		npcEyesComponentRefTD = setNpcEyesComponent;
	}
}

void NPCEyesProThread::SetEyesParameters(FNPCEyesParametersPro setEyesParameters)
{
	npcEyesParametersTD = setEyesParameters;
}

FHitResult NPCEyesProThread::CheckLineTraceTD(FVector startV, FVector& endV, bool setTraceComplex, AActor* setIgnoreActor) const
{
	FCollisionQueryParams traceParams("traceName", setTraceComplex, setIgnoreActor);

	traceParams.bReturnPhysicalMaterial = false;

	FHitResult hit(ForceInit);
	ownerActor->GetWorld()->LineTraceSingleByChannel(hit, startV, endV, npcEyesParametersTD.collisionChannel, traceParams);

	return hit;
}

TArray<FSeeEnemiesDataPro> NPCEyesProThread::GetSeeEnemiesData(TArray<AActor*> setEnemiesActors, FNPCEyesParametersPro setEyesParameters)
{
	//   m_mutex.Lock();

	TArray<FSeeEnemiesDataPro> seeEnemiesData_;

	// ContinueThread();
	// m_mutex.Unlock();


	if (m_mutex.TryLock())
	{
		enemiesActorsArrTD.Empty();
		enemiesActorsArrTD = setEnemiesActors;
		seeEnemiesData_ = seeEnemiesDataArr;
		npcEyesParametersTD = setEyesParameters;

		ContinueThread();
		m_mutex.Unlock();
	}

	return seeEnemiesData_;
}
