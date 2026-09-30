#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZSWorldDirector.generated.h"

class AZSSafeZone;
class AZSDangerZone;
class AZSZombie;
class APawn;
class UNavigationSystemV1;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnNewDay, int32, DayNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnDayPhase);

/**
 * Per-world brain of the survival loop:
 *  - the day/night clock,
 *  - the registry of loaded safe zones and danger zones (works with World Partition: only
 *    loaded actors register),
 *  - the zombie population, streamed around each player instead of being placed by hand,
 *    which is what keeps a very large map affordable.
 */
UCLASS()
class ZOMBIESURVIVAL_API UZSWorldDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UZSWorldDirector* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// --- Clock --------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "ZS|Time")
	FZSOnNewDay OnNewDay;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Time")
	FZSOnDayPhase OnNightfall;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Time")
	FZSOnDayPhase OnDawn;

	/** 0..24 */
	UFUNCTION(BlueprintPure, Category = "ZS|Time")
	float GetTimeOfDay() const { return TimeOfDay; }

	UFUNCTION(BlueprintPure, Category = "ZS|Time")
	int32 GetDayNumber() const { return DayNumber; }

	/** 0 in daylight, 1 in full night, smooth over dusk (19-21h) and dawn (4-6h). */
	UFUNCTION(BlueprintPure, Category = "ZS|Time")
	float GetNightAlpha() const;

	UFUNCTION(BlueprintPure, Category = "ZS|Time")
	bool IsNight() const { return GetNightAlpha() > 0.5f; }

	UFUNCTION(BlueprintPure, Category = "ZS|Time")
	FText GetTimeText() const;

	// --- Zones --------------------------------------------------------------------------------

	void RegisterSafeZone(AZSSafeZone* Zone);
	void UnregisterSafeZone(AZSSafeZone* Zone);
	void RegisterDangerZone(AZSDangerZone* Zone);
	void UnregisterDangerZone(AZSDangerZone* Zone);

	/** True when Location is inside any loaded safe zone (grown by Margin). */
	bool IsInSafeZone(const FVector& Location, float Margin = 0.f) const;
	AZSSafeZone* FindSafeZone(const FVector& Location) const;

	/** Zombie density multiplier at a location (1 by default, highest overlapping danger zone otherwise). */
	float GetDensityAt(const FVector& Location) const;

	// --- Zombies ------------------------------------------------------------------------------

	void RegisterZombie(AZSZombie* Zombie);
	int32 GetZombieCount() const { return Zombies.Num(); }
	int32 CountZombiesNear(const FVector& Location, float Radius) const;

private:
	void UpdateClock(float DeltaTime);
	void UpdateSpawning(float DeltaTime);
	bool TrySpawnNear(const APawn& Player, UNavigationSystemV1& Nav);

	float TimeOfDay = 8.f;
	int32 DayNumber = 1;
	bool bWasNight = false;
	float SpawnTimer = 0.f;

	TArray<TWeakObjectPtr<AZSSafeZone>> SafeZones;
	TArray<TWeakObjectPtr<AZSDangerZone>> DangerZones;
	TArray<TWeakObjectPtr<AZSZombie>> Zombies;
};
