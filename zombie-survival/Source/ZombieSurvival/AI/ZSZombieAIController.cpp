#include "AI/ZSZombieAIController.h"
#include "AI/ZSZombie.h"
#include "Core/ZSWorldDirector.h"
#include "Player/ZSCharacter.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AZSZombieAIController::AZSZombieAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1800.f;
	SightConfig->LoseSightRadius = 2100.f;
	SightConfig->PeripheralVisionAngleDegrees = 75.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 400.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 6000.f; // each noise event caps its own range
	HearingConfig->SetMaxAge(4.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AZSZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const AZSZombie* Zombie = Cast<AZSZombie>(InPawn))
	{
		// Zombies see further at night.
		const UZSWorldDirector* Director = UZSWorldDirector::Get(this);
		const float Night = Director ? Director->GetNightAlpha() : 0.f;
		SightConfig->SightRadius = Zombie->SightRadius * (1.f + 0.4f * Night);
		SightConfig->LoseSightRadius = SightConfig->SightRadius + 300.f;
		Perception->ConfigureSense(*SightConfig);
	}

	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AZSZombieAIController::OnTargetPerceived);

	// Staggered start so a horde does not think on the same frame.
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AZSZombieAIController::Think, 0.2f, true, FMath::FRandRange(0.f, 0.5f));
}

void AZSZombieAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	Super::OnUnPossess();
}

bool AZSZombieAIController::IsValidTarget(const AActor* Candidate) const
{
	const AZSCharacter* Player = Cast<AZSCharacter>(Candidate);
	if (!Player || Player->IsDead())
	{
		return false;
	}

	// Guards and walls: nobody hunts you inside a safe zone.
	const UZSWorldDirector* Director = UZSWorldDirector::Get(this);
	return !(Director && Director->IsInSafeZone(Player->GetActorLocation()));
}

void AZSZombieAIController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	AZSZombie* Zombie = Cast<AZSZombie>(GetPawn());
	if (!Zombie || Zombie->IsDead() || !IsValidTarget(Actor))
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bSight = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();

	if (bSight)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Target = Actor;
			bTargetVisible = true;
			bInvestigating = false;
			LastKnownLocation = Actor->GetActorLocation();
			Zombie->SetAlerted(true);
		}
		else if (Target.Get() == Actor)
		{
			bTargetVisible = false;
			TargetLostTime = Now;
			LastKnownLocation = Stimulus.StimulusLocation;
		}
	}
	else if (Stimulus.WasSuccessfullySensed() && !Target.IsValid())
	{
		// Heard something (gunshot, fight): go and look.
		bInvestigating = true;
		InvestigateLocation = Stimulus.StimulusLocation;
		InvestigateUntil = Now + 10.f;
		Zombie->SetAlerted(true);
	}
}

void AZSZombieAIController::ClearTarget()
{
	Target.Reset();
	bTargetVisible = false;
}

void AZSZombieAIController::Think()
{
	AZSZombie* Zombie = Cast<AZSZombie>(GetPawn());
	if (!Zombie || Zombie->IsDead())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	if (Zombie->IsStaggered())
	{
		StopMovement();
		return;
	}

	AActor* Prey = Target.Get();
	if (Prey && !IsValidTarget(Prey))
	{
		ClearTarget();
		Prey = nullptr;
		Zombie->SetAlerted(false);
	}

	if (Prey)
	{
		const FVector Here = Zombie->GetActorLocation();
		const FVector There = Prey->GetActorLocation();

		if (bTargetVisible)
		{
			LastKnownLocation = There;
		}
		else if (Now - TargetLostTime > LoseTargetSeconds)
		{
			// Lost them: check where we last saw them, then go back to wandering.
			bInvestigating = true;
			InvestigateLocation = LastKnownLocation;
			InvestigateUntil = Now + 8.f;
			ClearTarget();
			return;
		}

		const bool bInReach = FVector::Dist2D(Here, There) <= Zombie->AttackRange && FMath::Abs(There.Z - Here.Z) < 160.f;
		if (bTargetVisible && bInReach)
		{
			StopMovement();
			FaceTowards(There, 0.2f);
			if (Now >= NextAttackTime)
			{
				NextAttackTime = Now + Zombie->AttackInterval;
				Zombie->OnAttackStarted();
				UGameplayStatics::ApplyDamage(Prey, Zombie->AttackDamage, this, Zombie, UDamageType::StaticClass());
			}
		}
		else
		{
			MoveToward(bTargetVisible ? There : LastKnownLocation, Zombie->AttackRange * 0.6f, Now);
		}
		return;
	}

	if (bInvestigating)
	{
		if (FVector::Dist2D(Zombie->GetActorLocation(), InvestigateLocation) < 200.f || Now > InvestigateUntil)
		{
			bInvestigating = false;
			Zombie->SetAlerted(false);
			NextWanderTime = Now + FMath::FRandRange(2.f, 5.f);
		}
		else
		{
			MoveToward(InvestigateLocation, 120.f, Now);
		}
		return;
	}

	Wander(Now);
}

void AZSZombieAIController::Wander(float Now)
{
	if (Now < NextWanderTime)
	{
		return;
	}
	NextWanderTime = Now + FMath::FRandRange(4.f, 12.f);

	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav || !GetPawn())
	{
		return;
	}

	FNavLocation Destination;
	if (Nav->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 1500.f, Destination))
	{
		const UZSWorldDirector* Director = UZSWorldDirector::Get(this);
		if (!Director || !Director->IsInSafeZone(Destination.Location, 500.f))
		{
			MoveToward(Destination.Location, 100.f, Now);
		}
	}
}

void AZSZombieAIController::MoveToward(const FVector& Location, float AcceptanceRadius, float Now)
{
	// Re-pathing every think tick for a whole horde is expensive: only when the goal moved or after a delay.
	const bool bMoving = GetMoveStatus() == EPathFollowingStatus::Moving;
	if (bMoving && Now < NextRepathTime && FVector::DistSquared(Location, LastMoveGoal) < FMath::Square(200.f))
	{
		return;
	}

	NextRepathTime = Now + 0.5f;
	LastMoveGoal = Location;

	FAIMoveRequest Request(Location);
	Request.SetAcceptanceRadius(AcceptanceRadius);
	Request.SetUsePathfinding(true);
	Request.SetAllowPartialPath(true);
	Request.SetProjectGoalLocation(true);
	MoveTo(Request);
}

void AZSZombieAIController::FaceTowards(const FVector& Location, float DeltaTime)
{
	if (APawn* Pawn = GetPawn())
	{
		const FRotator Desired(0.f, (Location - Pawn->GetActorLocation()).Rotation().Yaw, 0.f);
		Pawn->SetActorRotation(FMath::RInterpTo(Pawn->GetActorRotation(), Desired, DeltaTime, 8.f));
	}
}
