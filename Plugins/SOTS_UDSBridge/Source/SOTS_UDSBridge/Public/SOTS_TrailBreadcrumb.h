#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTS_TrailBreadcrumb.generated.h"

class UAIPerceptionStimuliSourceComponent;
class USphereComponent;

/**
 * Lightweight breadcrumb actor for trail tracking.
 * Registers as an AI perception stimulus source and cleans up its own chain links.
 */
UCLASS()
class SOTS_UDSBRIDGE_API ASOTS_TrailBreadcrumb : public AActor
{
	GENERATED_BODY()

public:
	ASOTS_TrailBreadcrumb();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trail")
	TObjectPtr<ASOTS_TrailBreadcrumb> Prev = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trail")
	TObjectPtr<ASOTS_TrailBreadcrumb> Next = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
	float LifespanSeconds = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trail")
	double SpawnTimeSeconds = 0.0;

	// Optional: allows caller to request an initial hearing stimulus on spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	bool bEmitHearingEventOnSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float HearingLoudness = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	float HearingMaxRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	FName HearingTag = TEXT("TrailBreadcrumb");

	// Called externally to trigger a hearing event (after SpawnParams set the instigator).
	void EmitHearingEvent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Trail")
	TObjectPtr<USphereComponent> SenseSphere = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Trail")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource = nullptr;
};
