#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnInventoryChanged);

/**
 * Slot-based inventory plus a wallet. Used by the player and, as a "stock room with a cash drawer",
 * by every merchant.
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class ZOMBIESURVIVAL_API UZSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZSInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnInventoryChanged OnChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Inventory")
	int32 MaxSlots = 24;

	// --- Items --------------------------------------------------------------------------------

	/** Adds items. Returns how many did NOT fit. Durability <= 0 means "brand new". */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 AddItem(FName ItemId, int32 Quantity, int32 Durability = 0, int32 AmmoInMag = 0);

	/** Returns how many did NOT fit. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 AddStack(const FZSItemStack& Stack);

	/** Removes up to Quantity of an item across slots. Returns how many were removed. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 RemoveItem(FName ItemId, int32 Quantity);

	/** Removes up to Quantity from one slot; OutRemoved keeps the per-instance state (durability...). */
	bool RemoveFromSlot(int32 SlotIndex, int32 Quantity, FZSItemStack& OutRemoved);

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 CountItem(FName ItemId) const;

	/** Whether the whole stack would fit right now. */
	bool CanFit(const FZSItemStack& Stack) const;

	const TArray<FZSItemStack>& GetSlots() const { return Slots; }
	TArray<FZSItemStack>& GetMutableSlots() { return Slots; }

	// --- Money --------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "ZS|Economy")
	int32 GetMoney() const { return Money; }

	UFUNCTION(BlueprintCallable, Category = "ZS|Economy")
	void SetMoney(int32 NewMoney);

	UFUNCTION(BlueprintCallable, Category = "ZS|Economy")
	void AddMoney(int32 Amount);

	/** Returns false (and spends nothing) when the wallet is short. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Economy")
	bool TrySpendMoney(int32 Amount);

private:
	UPROPERTY(VisibleAnywhere, Category = "ZS|Inventory")
	TArray<FZSItemStack> Slots;

	UPROPERTY(VisibleAnywhere, Category = "ZS|Economy")
	int32 Money = 0;
};
