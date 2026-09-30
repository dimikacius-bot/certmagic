#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSHealthComponent.generated.h"

class UDamageType;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZSOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnDeath, AActor*, Killer);

/** Hit points for anything that can take engine damage (ApplyDamage / ApplyPointDamage). */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class ZOMBIESURVIVAL_API UZSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZSHealthComponent();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnDeath OnDeath;

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetHealth() const { return Health < 0.f ? MaxHealth : Health; }

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsDead() const { return bDead; }

	/** Sets max health and refills. Safe to call before BeginPlay. */
	void SetMaxHealth(float NewMax);

	/** Returns the amount actually healed. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	float Heal(float Amount);

	/** Brings a dead owner back with a fraction of its max health. */
	void Revive(float HealthFraction = 0.6f);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	/** Negative until initialised. */
	float Health = -1.f;
	bool bDead = false;
};
