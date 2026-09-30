#include "Core/ZSWorldDirector.h"
#include "Core/ZSSettings.h"
#include "Economy/ZSSafeZone.h"
#include "AI/ZSDangerZone.h"
#include "AI/ZSZombie.h"
#include "ZombieSurvival.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

UZSWorldDirector* UZSWorldDirector::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UZSWorldDirector>() : nullptr;
}

bool UZSWorldDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UZSWorldDirector::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	TimeOfDay = UZSSettings::Get()->StartHour;
	bWasNight = IsNight();
}

TStatId UZSWorldDirector::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZSWorldDirector, STATGROUP_Tickables);
}

void UZSWorldDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World || !World->HasBegunPlay())
	{
		return;
	}

	UpdateClock(DeltaTime);
	UpdateSpawning(DeltaTime);
}

// --- Clock ------------------------------------------------------------------------------------

void UZSWorldDirector::UpdateClock(float DeltaTime)
{
	const float DaySeconds = FMath::Max(1.f, UZSSettings::Get()->DayLengthMinutes * 60.f);
	TimeOfDay += DeltaTime / DaySeconds * 24.f;

	if (TimeOfDay >= 24.f)
	{
		TimeOfDay -= 24.f;
		++DayNumber;
		OnNewDay.Broadcast(DayNumber);
	}

	const bool bNight = IsNight();
	if (bNight != bWasNight)
	{
		bWasNight = bNight;
		if (bNight)
		{
			OnNightfall.Broadcast();
		}
		else
		{
			OnDawn.Broadcast();
		}
	}
}

float UZSWorldDirector::GetNightAlpha() const
{
	const float H = TimeOfDay;
	if (H >= 21.f || H <= 4.f)
	{
		return 1.f;
	}
	if (H > 19.f)
	{
		return (H - 19.f) / 2.f; // dusk
	}
	if (H > 4.f && H < 6.f)
	{
		return (6.f - H) / 2.f; // dawn
	}
	return 0.f;
}

FText UZSWorldDirector::GetTimeText() const
{
	const int32 Hours = FMath::FloorToInt(TimeOfDay) % 24;
	const int32 Minutes = FMath::FloorToInt((TimeOfDay - FMath::FloorToFloat(TimeOfDay)) * 60.f);
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Hours, Minutes));
}

// --- Zones ------------------------------------------------------------------------------------

void UZSWorldDirector::RegisterSafeZone(AZSSafeZone* Zone)
{
	if (Zone)
	{
		SafeZones.AddUnique(Zone);
	}
}

void UZSWorldDirector::UnregisterSafeZone(AZSSafeZone* Zone)
{
	SafeZones.Remove(Zone);
}

void UZSWorldDirector::RegisterDangerZone(AZSDangerZone* Zone)
{
	if (Zone)
	{
		DangerZones.AddUnique(Zone);
	}
}

void UZSWorldDirector::UnregisterDangerZone(AZSDangerZone* Zone)
{
	DangerZones.Remove(Zone);
}

AZSSafeZone* UZSWorldDirector::FindSafeZone(const FVector& Location) const
{
	for (const TWeakObjectPtr<AZSSafeZone>& Zone : SafeZones)
	{
		if (Zone.IsValid() && Zone->ContainsPoint(Location, 0.f))
		{
			return Zone.Get();
		}
	}
	return nullptr;
}

bool UZSWorldDirector::IsInSafeZone(const FVector& Location, float Margin) const
{
	for (const TWeakObjectPtr<AZSSafeZone>& Zone : SafeZones)
	{
		if (Zone.IsValid() && Zone->ContainsPoint(Location, Margin))
		{
			return true;
		}
	}
	return false;
}

float UZSWorldDirector::GetDensityAt(const FVector& Location) const
{
	float Density = 1.f;
	for (const TWeakObjectPtr<AZSDangerZone>& Zone : DangerZones)
	{
		if (Zone.IsValid() && Zone->ContainsPoint(Location))
		{
			Density = FMath::Max(Density, Zone->DensityMultiplier);
		}
	}
	return Density;
}

// --- Zombies ----------------------------------------------------------------------------------

void UZSWorldDirector::RegisterZombie(AZSZombie* Zombie)
{
	if (Zombie)
	{
		Zombies.AddUnique(Zombie);
	}
}

int32 UZSWorldDirector::CountZombiesNear(const FVector& Location, float Radius) const
{
	const float RadiusSq = FMath::Square(Radius);
	int32 Count = 0;
	for (const TWeakObjectPtr<AZSZombie>& Zombie : Zombies)
	{
		if (Zombie.IsValid() && !Zombie->IsDead() && FVector::DistSquared(Zombie->GetActorLocation(), Location) <= RadiusSq)
		{
			++Count;
		}
	}
	return Count;
}

namespace
{
	EZSZombieType PickZombieType(float NightAlpha)
	{
		const float WWalker = FMath::Lerp(60.f, 30.f, NightAlpha);
		const float WRunner = FMath::Lerp(25.f, 30.f, NightAlpha);
		const float WBrute = FMath::Lerp(6.f, 10.f, NightAlpha);
		const float WVolatile = NightAlpha > 0.5f ? 25.f * NightAlpha : 0.f;

		float Roll = FMath::FRand() * (WWalker + WRunner + WBrute + WVolatile);
		if ((Roll -= WWalker) < 0.f) { return EZSZombieType::Walker; }
		if ((Roll -= WRunner) < 0.f) { return EZSZombieType::Runner; }
		if ((Roll -= WBrute) < 0.f) { return EZSZombieType::Brute; }
		return EZSZombieType::Volatile;
	}
}

void UZSWorldDirector::UpdateSpawning(float DeltaTime)
{
	const UZSSettings* Settings = UZSSettings::Get();

	SpawnTimer += DeltaTime;
	if (SpawnTimer < Settings->SpawnIntervalSeconds)
	{
		return;
	}
	SpawnTimer = 0.f;

	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!Nav)
	{
		return;
	}

	Zombies.RemoveAll([](const TWeakObjectPtr<AZSZombie>& Z) { return !Z.IsValid(); });

	TArray<const APawn*> Players;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PC = It->Get())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				Players.Add(Pawn);
			}
		}
	}
	if (Players.IsEmpty())
	{
		return;
	}

	// Despawn zombies that are far from everyone and out of sight.
	const float DespawnSq = FMath::Square(Settings->DespawnRadius);
	for (const TWeakObjectPtr<AZSZombie>& ZombiePtr : Zombies)
	{
		AZSZombie* Zombie = ZombiePtr.Get();
		if (!Zombie)
		{
			continue;
		}

		float NearestSq = TNumericLimits<float>::Max();
		for (const APawn* Player : Players)
		{
			NearestSq = FMath::Min(NearestSq, FVector::DistSquared(Zombie->GetActorLocation(), Player->GetActorLocation()));
		}

		const bool bFar = NearestSq > DespawnSq;
		const bool bDeadAndOld = Zombie->IsDead() && NearestSq > FMath::Square(3000.f);
		if ((bFar || bDeadAndOld) && !Zombie->WasRecentlyRendered(1.f))
		{
			Zombie->Destroy();
		}
	}

	// Top up the budget around each player, one spawn per player per tick.
	const float Night = GetNightAlpha();
	for (const APawn* Player : Players)
	{
		if (Zombies.Num() >= Settings->MaxZombiesInWorld)
		{
			break;
		}

		const FVector Origin = Player->GetActorLocation();
		const float BaseBudget = FMath::Lerp(static_cast<float>(Settings->DayZombieBudget), static_cast<float>(Settings->NightZombieBudget), Night);
		const int32 Budget = FMath::RoundToInt(BaseBudget * GetDensityAt(Origin));

		if (CountZombiesNear(Origin, Settings->DespawnRadius) < Budget)
		{
			TrySpawnNear(*Player, *Nav);
		}
	}
}

bool UZSWorldDirector::TrySpawnNear(const APawn& Player, UNavigationSystemV1& Nav)
{
	const UZSSettings* Settings = UZSSettings::Get();
	UWorld* World = GetWorld();
	const FVector Origin = Player.GetActorLocation();

	FVector EyeLocation;
	FRotator EyeRotation;
	Player.GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ZSSpawnVisibility), false, &Player);

	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		const float Angle = FMath::FRand() * TWO_PI;
		const float Distance = FMath::RandRange(Settings->SpawnRadiusMin, Settings->SpawnRadiusMax);
		const FVector Candidate = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;

		FNavLocation NavLocation;
		if (!Nav.ProjectPointToNavigation(Candidate, NavLocation, FVector(600.f, 600.f, 1500.f)))
		{
			continue;
		}

		const FVector Spot = NavLocation.Location + FVector(0.f, 0.f, 95.f);

		if (IsInSafeZone(Spot, Settings->SafeZoneSpawnMargin))
		{
			continue;
		}

		// Do not pop zombies into existence in plain view.
		if (FVector::Dist(Spot, EyeLocation) < Settings->NoSpawnIfVisibleWithin)
		{
			const bool bOccluded = World->LineTraceTestByChannel(EyeLocation, Spot, ECC_Visibility, TraceParams);
			if (!bOccluded)
			{
				continue;
			}
		}

		const FTransform SpawnTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Spot);

		UClass* ZombieClass = Settings->ZombieClass.LoadSynchronous();
		if (!ZombieClass)
		{
			ZombieClass = AZSZombie::StaticClass();
		}

		if (AZSZombie* Zombie = World->SpawnActorDeferred<AZSZombie>(ZombieClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding))
		{
			Zombie->ZombieType = PickZombieType(GetNightAlpha());
			Zombie->FinishSpawning(SpawnTransform);
			return true;
		}
	}

	return false;
}
