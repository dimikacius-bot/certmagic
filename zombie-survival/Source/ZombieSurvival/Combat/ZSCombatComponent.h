#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSCombatComponent.generated.h"

class UZSInventoryComponent;
class AZSCharacter;
class AZSZombie;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnEquipChanged);
/** Hook for animation / SFX / VFX in Blueprint. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnAttack, bool, bRanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnWeaponBroke, FName, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnReloadStarted);

/**
 * Player weapon handling: which inventory slot is equipped, melee sweeps, hitscan firearms,
 * reloads from the inventory's ammo, durability wear and the noise every attack makes.
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class ZOMBIESURVIVAL_API UZSCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZSCombatComponent();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Combat")
	FZSOnEquipChanged OnEquipChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Combat")
	FZSOnAttack OnAttack;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Combat")
	FZSOnWeaponBroke OnWeaponBroke;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Combat")
	FZSOnReloadStarted OnReloadStarted;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Unarmed")
	float UnarmedDamage = 8.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Unarmed")
	float UnarmedRange = 140.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Unarmed")
	float UnarmedInterval = 0.7f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Unarmed")
	float UnarmedStamina = 6.f;

	/** Damage multiplier for shots landing in the top of a zombie's capsule. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat")
	float HeadshotMultiplier = 2.5f;

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	bool Equip(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void Unequip();

	/** Cycles through unarmed and every weapon in the inventory. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void CycleWeapon(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void AutoEquipBest();

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	bool PrimaryAttack();

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void StartReload();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool IsReloading() const { return bReloading; }

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	int32 GetEquippedSlot() const { return EquippedSlot; }

	const FZSItemStack* GetEquippedStack() const;
	const FZSItemRow* GetEquippedRow() const;

protected:
	virtual void BeginPlay() override;

private:
	FZSItemStack* GetEquippedMutable();
	void GetAim(FVector& OutLocation, FVector& OutDirection) const;

	bool DoMelee(AZSCharacter& Owner, const FZSItemRow* Row);
	bool DoFire(AZSCharacter& Owner, const FZSItemRow& Row);
	void ConsumeDurability(int32 Amount);
	void ReportNoise(float Range, FName Tag);
	void CancelReload();

	UFUNCTION()
	void FinishReload();

	UFUNCTION()
	void HandleInventoryChanged();

	UPROPERTY()
	TObjectPtr<UZSInventoryComponent> Inventory;

	int32 EquippedSlot = INDEX_NONE;
	FName EquippedId;
	float NextAttackTime = 0.f;
	bool bReloading = false;
	FTimerHandle ReloadTimer;
};
