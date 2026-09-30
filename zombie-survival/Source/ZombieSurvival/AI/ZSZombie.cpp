#include "AI/ZSZombie.h"
#include "AI/ZSZombieAIController.h"
#include "Combat/ZSHealthComponent.h"
#include "Core/ZSPlaceholder.h"
#include "Core/ZSWorldDirector.h"
#include "Economy/ZSLootTables.h"
#include "Economy/ZSPickup.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AZSZombie::AZSZombie()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AZSZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // hitscan + interaction traces

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 300.f, 0.f);
	Movement->bUseRVOAvoidance = true; // keep hordes from stacking on one spot
	Movement->AvoidanceConsiderationRadius = 200.f;
	Movement->MaxWalkSpeed = PatrolSpeed;

	Health = CreateDefaultSubobject<UZSHealthComponent>(TEXT("Health"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f));
	ZSPlaceholder::SetShape(PlaceholderBody, EZSBasicShape::Cylinder);
}

void AZSZombie::PostInitializeComponents()
{
	// Stats must exist before Super spawns the AI controller, which reads them on possess.
	ApplyTypeStats();

	Health->OnDeath.AddDynamic(this, &AZSZombie::HandleDeath);
	Health->OnHealthChanged.AddDynamic(this, &AZSZombie::HandleHealthChanged);

	Super::PostInitializeComponents();
}

void AZSZombie::BeginPlay()
{
	Super::BeginPlay();

	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->RegisterZombie(this);
	}

	if (GetMesh() && GetMesh()->GetSkeletalMeshAsset())
	{
		PlaceholderBody->SetVisibility(false);
	}
}

void AZSZombie::ApplyTypeStats()
{
	struct FStats
	{
		float Health, Damage, Range, Interval, Patrol, Chase, Sight, Scale;
		FLinearColor Color;
	};

	FStats S;
	switch (ZombieType)
	{
	case EZSZombieType::Runner:
		S = { 60.f, 10.f, 150.f, 0.9f, 130.f, 420.f, 2200.f, 1.f, FLinearColor(0.55f, 0.55f, 0.2f) };
		break;
	case EZSZombieType::Brute:
		S = { 320.f, 35.f, 185.f, 1.8f, 90.f, 260.f, 1500.f, 1.2f, FLinearColor(0.4f, 0.13f, 0.1f) };
		break;
	case EZSZombieType::Volatile:
		S = { 140.f, 28.f, 160.f, 0.8f, 200.f, 620.f, 3500.f, 1.f, FLinearColor(0.5f, 0.2f, 0.6f) };
		break;
	default:
		S = { 80.f, 12.f, 150.f, 1.3f, 90.f, 210.f, 1800.f, 1.f, FLinearColor(0.35f, 0.45f, 0.3f) };
		break;
	}

	const float SpeedJitter = FMath::RandRange(0.9f, 1.1f);

	Health->SetMaxHealth(S.Health);
	AttackDamage = S.Damage;
	AttackRange = S.Range;
	AttackInterval = S.Interval;
	PatrolSpeed = S.Patrol * SpeedJitter;
	ChaseSpeed = S.Chase * SpeedJitter;
	SightRadius = S.Sight;

	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
	if (S.Scale != 1.f)
	{
		SetActorScale3D(FVector(S.Scale));
	}
	ZSPlaceholder::SetColor(PlaceholderBody, S.Color);
}

bool AZSZombie::IsDead() const
{
	return Health && Health->IsDead();
}

bool AZSZombie::IsStaggered() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < StaggerUntil;
}

void AZSZombie::ApplyStagger(float Seconds)
{
	if (IsDead())
	{
		return;
	}

	// Brutes barely flinch.
	const float Scaled = ZombieType == EZSZombieType::Brute ? Seconds * 0.3f : Seconds;
	StaggerUntil = FMath::Max(StaggerUntil, GetWorld()->GetTimeSeconds() + Scaled);
	OnHitReaction();
}

void AZSZombie::SetAlerted(bool bAlerted)
{
	GetCharacterMovement()->MaxWalkSpeed = bAlerted ? ChaseSpeed : PatrolSpeed;
}

void AZSZombie::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// Being shot or hit wakes the zombie up even if it has not seen its attacker.
	SetAlerted(true);
}

void AZSZombie::HandleDeath(AActor* Killer)
{
	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetMesh() && GetMesh()->GetSkeletalMeshAsset())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}
	else
	{
		// Tip the stand-in cylinder over.
		PlaceholderBody->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		PlaceholderBody->AddRelativeLocation(FVector(0.f, 0.f, -58.f));
	}

	DropLoot();
	OnDied();
	SetLifeSpan(20.f);
}

void AZSZombie::DropLoot()
{
	const FZSLootResult Loot = ZSLoot::Roll(this, EZSLootTable::Zombie);

	for (const FZSItemStack& Stack : Loot.Items)
	{
		const FVector Offset(FMath::RandRange(-40.f, 40.f), FMath::RandRange(-40.f, 40.f), 0.f);
		AZSPickup::SpawnItem(GetWorld(), GetActorLocation() + Offset, Stack, 300.f);
	}
	if (Loot.Cash > 0)
	{
		AZSPickup::SpawnCash(GetWorld(), GetActorLocation(), Loot.Cash, 300.f);
	}
}
