#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ZSSettings.generated.h"

class UDataTable;
class AZSZombie;

/** Project-wide tunables (Project Settings > Game > Zombie Survival). Stored in DefaultGame.ini. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Zombie Survival"))
class ZOMBIESURVIVAL_API UZSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UZSSettings();

	static const UZSSettings* Get() { return GetDefault<UZSSettings>(); }

	// --- Time -------------------------------------------------------------------------------

	/** Real-time minutes for one full 24h in-game day. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "1.0"))
	float DayLengthMinutes = 40.f;

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartHour = 8.f;

	// --- Zombie population (streamed around each player, never placed by hand) ---------------

	UPROPERTY(Config, EditAnywhere, Category = "Zombies")
	int32 DayZombieBudget = 30;

	UPROPERTY(Config, EditAnywhere, Category = "Zombies")
	int32 NightZombieBudget = 60;

	UPROPERTY(Config, EditAnywhere, Category = "Zombies")
	int32 MaxZombiesInWorld = 150;

	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "cm"))
	float SpawnRadiusMin = 3000.f;

	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "cm"))
	float SpawnRadiusMax = 6000.f;

	/** Zombies further than this from every player (and unseen) are removed. Keep below the navigation invoker radius. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "cm"))
	float DespawnRadius = 8000.f;

	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "s"))
	float SpawnIntervalSeconds = 0.6f;

	/** No spawns this close to a safe zone. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "cm"))
	float SafeZoneSpawnMargin = 2500.f;

	/** Zombies are never spawned in plain view of the player closer than this. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombies", meta = (Units = "cm"))
	float NoSpawnIfVisibleWithin = 4500.f;

	/** Optional Blueprint subclass of AZSZombie (with your skeletal mesh + anim BP). Empty = C++ class with placeholder body. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombies")
	TSoftClassPtr<AZSZombie> ZombieClass;

	// --- Economy -----------------------------------------------------------------------------

	/** Fraction of the player's cash dropped where they die (recoverable). */
	UPROPERTY(Config, EditAnywhere, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathCashPenalty = 0.2f;

	/** Optional DataTable (row struct FZSItemRow) whose rows add to / override the built-in item catalog. */
	UPROPERTY(Config, EditAnywhere, Category = "Economy")
	TSoftObjectPtr<UDataTable> ItemOverrideTable;
};
