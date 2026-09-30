#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/TimerHandle.h"
#include "Combat/ZSCombatComponent.h"
#include "Combat/ZSHealthComponent.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSCharacter.generated.h"

class UCameraComponent;
class UNavigationInvokerComponent;
class UAIPerceptionStimuliSourceComponent;
class AZSSafeZone;

/**
 * First-person survivor with Dying-Light-style movement: stamina-limited sprint, crouch and
 * ledge-grab parkour (hold Jump into a wall to vault low obstacles or climb up to ~2.3 m).
 * Owns the inventory / wallet, health and weapon handling.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZSCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Components ---------------------------------------------------------------------------

	UZSInventoryComponent* GetInventory() const { return Inventory; }
	UZSHealthComponent* GetHealthComponent() const { return Health; }
	UZSCombatComponent* GetCombat() const { return Combat; }

	// --- State --------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ZS|Player")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category = "ZS|Player")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "ZS|Player")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "ZS|Player")
	bool IsExhausted() const { return bExhausted; }

	UFUNCTION(BlueprintPure, Category = "ZS|Player")
	bool IsInSafeZone() const;

	AZSSafeZone* GetCurrentSafeZone() const;

	/** Spends stamina; false when there is not enough left to attempt the action. */
	bool TrySpendStamina(float Cost);

	void EnterSafeZone(AZSSafeZone* Zone);
	void ExitSafeZone(AZSSafeZone* Zone);

	/** Pushes a line to the on-screen message feed. */
	void Notify(const FText& Message, const FLinearColor& Color = FLinearColor::White);

	AActor* GetFocusedActor() const { return FocusedActor.Get(); }
	FText GetFocusPrompt() const;

	/** Uses the best-fitting healing item from the inventory. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Player")
	bool UseHealingItem();

	float GetSecondsSinceDamage() const;
	bool IsUIBlocking() const;

	// --- Blueprint hooks (animation / audio) --------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "ZS|Player")
	void OnMantleStarted(float LedgeHeight);

	UFUNCTION(BlueprintImplementableEvent, Category = "ZS|Player")
	void OnDied();

	// --- Tuning -------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "ZS|Movement") float WalkSpeed = 350.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Movement") float SprintSpeed = 650.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Movement") float CrouchSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float MaxStamina = 100.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float SprintDrainPerSecond = 14.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float StaminaRegenPerSecond = 22.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float StaminaRegenDelay = 0.8f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float ExhaustRecoverAt = 25.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float JumpStaminaCost = 8.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Stamina") float MantleStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Parkour") float MinMantleHeight = 50.f;
	/** Up to this height a mantle is a quick vault. */
	UPROPERTY(EditAnywhere, Category = "ZS|Parkour") float VaultMaxHeight = 110.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Parkour") float MaxMantleHeight = 230.f;
	UPROPERTY(EditAnywhere, Category = "ZS|Parkour") float MantleReach = 70.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Interaction") float InteractRange = 300.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Respawn") float RespawnDelay = 5.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Start") int32 StartingMoney = 120;
	UPROPERTY(EditAnywhere, Category = "ZS|Start") TArray<FZSItemStack> StartingItems;

protected:
	virtual void BeginPlay() override;

private:
	// Input
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void JumpPressed();
	void JumpReleased();
	void SprintPressed() { bWantsSprint = true; }
	void SprintReleased() { bWantsSprint = false; }
	void CrouchPressed();
	void CrouchReleased();
	void AttackPressed();
	void AttackReleased() { bAttackHeld = false; }
	void ReloadPressed();
	void InteractPressed();
	void HealPressed();
	void NextWeaponPressed();
	void PrevWeaponPressed();
	void HolsterPressed();
	void ToggleInventoryPressed();

	// Systems
	void UpdateSprintAndStamina(float DeltaSeconds);
	void UpdateFocus();
	bool TryStartMantle();
	void TickMantle(float DeltaSeconds);
	void Respawn();

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UZSInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UZSHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UZSCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNavigationInvokerComponent> NavInvoker;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	TWeakObjectPtr<AActor> FocusedActor;
	TArray<TWeakObjectPtr<AZSSafeZone>> SafeZones;

	float Stamina = 100.f;
	float LastStaminaUseTime = 0.f;
	float LastDamageTime = -1000.f;
	float LastHealth = -1.f;
	float FocusTimer = 0.f;
	float NextMantleProbeTime = 0.f;
	float NextMantleTime = 0.f;

	bool bWantsSprint = false;
	bool bExhausted = false;
	bool bJumpHeld = false;
	bool bAttackHeld = false;
	bool bDead = false;

	// Mantle / vault
	bool bMantling = false;
	float MantleElapsed = 0.f;
	float MantleDuration = 0.4f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleEnd = FVector::ZeroVector;
	FVector MantleForward = FVector::ForwardVector;

	FTimerHandle RespawnTimer;
};
