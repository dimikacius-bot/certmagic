#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZSZombieAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AZSZombie;

/**
 * Code-only zombie brain (no Behavior Tree asset needed): wander -> investigate noises -> chase what
 * it sees -> attack. Gunshots and fights are heard from far away, which is what makes hordes gather.
 * Players standing inside a safe zone are ignored.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZSZombieAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	void Think();
	void Wander(float Now);
	void ClearTarget();
	void MoveToward(const FVector& Location, float AcceptanceRadius, float Now);
	void FaceTowards(const FVector& Location, float DeltaTime);
	bool IsValidTarget(const AActor* Candidate) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	TWeakObjectPtr<AActor> Target;
	bool bTargetVisible = false;
	float TargetLostTime = 0.f;
	FVector LastKnownLocation = FVector::ZeroVector;

	bool bInvestigating = false;
	FVector InvestigateLocation = FVector::ZeroVector;
	float InvestigateUntil = 0.f;

	FVector LastMoveGoal = FVector::ZeroVector;
	float NextRepathTime = 0.f;
	float NextWanderTime = 0.f;
	float NextAttackTime = 0.f;

	/** Seconds without sight of the target before the zombie gives up and investigates instead. */
	float LoseTargetSeconds = 6.f;

	FTimerHandle ThinkTimer;
};
