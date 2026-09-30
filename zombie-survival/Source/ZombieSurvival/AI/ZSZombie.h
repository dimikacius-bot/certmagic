#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZSZombie.generated.h"

class UZSHealthComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EZSZombieType : uint8
{
	/** Slow shambler. Dangerous in numbers. */
	Walker,
	/** Fast and fragile. */
	Runner,
	/** Slow tank that hits like a truck. */
	Brute,
	/** Night only. Sees far, runs almost as fast as you sprint. */
	Volatile
};

/**
 * The infected. Stats come from the type (see ApplyTypeStats). Subclass in Blueprint to assign a
 * skeletal mesh + animation Blueprint and use the BlueprintImplementableEvents for anim / SFX.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSZombie : public ACharacter
{
	GENERATED_BODY()

public:
	AZSZombie();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie", meta = (ExposeOnSpawn = true))
	EZSZombieType ZombieType = EZSZombieType::Walker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie")
	TObjectPtr<UZSHealthComponent> Health;

	// Runtime stats, filled from ZombieType in PostInitializeComponents.
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float AttackDamage = 12.f;
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float AttackRange = 150.f;
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float AttackInterval = 1.3f;
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float PatrolSpeed = 90.f;
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float ChaseSpeed = 210.f;
	UPROPERTY(BlueprintReadOnly, Category = "Zombie") float SightRadius = 1800.f;

	UFUNCTION(BlueprintPure, Category = "Zombie")
	bool IsDead() const;

	bool IsStaggered() const;
	void ApplyStagger(float Seconds);

	/** Called by the AI when it swings. Drive an attack montage from here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie")
	void OnAttackStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie")
	void OnHitReaction();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie")
	void OnDied();

	/** Speed used while hunting; the AI toggles this. */
	void SetAlerted(bool bAlerted);

	virtual void PostInitializeComponents() override;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyTypeStats();
	void DropLoot();

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	float StaggerUntil = 0.f;
};
