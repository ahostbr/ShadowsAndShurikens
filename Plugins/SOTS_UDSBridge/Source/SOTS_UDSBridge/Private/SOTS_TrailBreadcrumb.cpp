#include "SOTS_TrailBreadcrumb.h"

#include "Components/SphereComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"

ASOTS_TrailBreadcrumb::ASOTS_TrailBreadcrumb()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SenseSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SenseSphere"));
	SetRootComponent(SenseSphere);
	SenseSphere->SetSphereRadius(100.f);
	SenseSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SenseSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	SenseSphere->SetHiddenInGame(true);

	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterWithPerceptionSystem();
}

void ASOTS_TrailBreadcrumb::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		SpawnTimeSeconds = World->GetTimeSeconds();
	}

	SetLifeSpan(LifespanSeconds);

	if (bEmitHearingEventOnSpawn)
	{
		EmitHearingEvent();
	}
}

void ASOTS_TrailBreadcrumb::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Prev)
	{
		Prev->Next = Next;
	}
	if (Next)
	{
		Next->Prev = Prev;
	}

	Super::EndPlay(EndPlayReason);
}

void ASOTS_TrailBreadcrumb::EmitHearingEvent()
{
	if (UWorld* World = GetWorld())
	{
		UAISense_Hearing::ReportNoiseEvent(
			World,
			GetActorLocation(),
			HearingLoudness,
			GetInstigator(),
			HearingMaxRange,
			HearingTag);
	}
}
