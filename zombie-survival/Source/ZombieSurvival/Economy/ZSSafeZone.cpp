#include "Economy/ZSSafeZone.h"
#include "AI/ZSZombie.h"
#include "Core/ZSGameMode.h"
#include "Core/ZSWorldDirector.h"
#include "Player/ZSCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "TimerManager.h"

AZSSafeZone::AZSSafeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	Zone->SetBoxExtent(FVector(3000.f, 3000.f, 1000.f));
	Zone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Zone->SetGenerateOverlapEvents(true);
	Zone->ShapeColor = FColor(60, 200, 90);
	RootComponent = Zone;

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(Zone);
}

void AZSSafeZone::BeginPlay()
{
	Super::BeginPlay();

	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->RegisterSafeZone(this);
	}

	Zone->OnComponentBeginOverlap.AddDynamic(this, &AZSSafeZone::OnZoneBeginOverlap);
	Zone->OnComponentEndOverlap.AddDynamic(this, &AZSSafeZone::OnZoneEndOverlap);

	if (bGuardsKillZombies)
	{
		GetWorldTimerManager().SetTimer(GuardTimer, this, &AZSSafeZone::GuardTick, FMath::Max(0.1f, GuardInterval), true);
	}

	// Anyone already standing inside at load time.
	TArray<AActor*> Inside;
	Zone->GetOverlappingActors(Inside, AZSCharacter::StaticClass());
	for (AActor* Actor : Inside)
	{
		OnZoneBeginOverlap(Zone, Actor, nullptr, 0, false, FHitResult());
	}
}

void AZSSafeZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->UnregisterSafeZone(this);
	}
	GetWorldTimerManager().ClearTimer(GuardTimer);
	Super::EndPlay(EndPlayReason);
}

bool AZSSafeZone::ContainsPoint(const FVector& WorldPoint, float Margin) const
{
	const FTransform NoScale(Zone->GetComponentRotation(), Zone->GetComponentLocation());
	const FVector Local = NoScale.InverseTransformPosition(WorldPoint);
	const FVector Extent = Zone->GetScaledBoxExtent() + FVector(Margin);
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

FTransform AZSSafeZone::GetRespawnTransform() const
{
	return RespawnPoint->GetComponentTransform();
}

void AZSSafeZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AZSCharacter* Player = Cast<AZSCharacter>(OtherActor))
	{
		Player->EnterSafeZone(this);
		if (AZSGameMode* GameMode = GetWorld()->GetAuthGameMode<AZSGameMode>())
		{
			GameMode->SetLastSafeZone(this);
		}
	}
}

void AZSSafeZone::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AZSCharacter* Player = Cast<AZSCharacter>(OtherActor))
	{
		Player->ExitSafeZone(this);
	}
}

void AZSSafeZone::GuardTick()
{
	TArray<AActor*> Intruders;
	Zone->GetOverlappingActors(Intruders, AZSZombie::StaticClass());

	for (AActor* Actor : Intruders)
	{
		AZSZombie* Zombie = Cast<AZSZombie>(Actor);
		if (!Zombie || Zombie->IsDead())
		{
			continue;
		}

		UGameplayStatics::ApplyDamage(Zombie, GuardDamage, nullptr, this, UDamageType::StaticClass());

#if !UE_BUILD_SHIPPING
		// Placeholder for guard tracers / muzzle flashes.
		DrawDebugLine(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 600.f), Zombie->GetActorLocation(), FColor::Red, false, 0.08f, 0, 2.f);
#endif
	}
}
