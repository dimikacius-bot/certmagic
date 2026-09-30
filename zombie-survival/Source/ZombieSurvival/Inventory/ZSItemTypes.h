#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ZSItemTypes.generated.h"

UENUM(BlueprintType)
enum class EZSItemCategory : uint8
{
	Junk,
	Material,
	Food,
	Medical,
	Melee,
	Firearm,
	Ammo
};

/**
 * Static definition of an item. The built-in catalog lives in ZSItemSubsystem.cpp; a DataTable using
 * this row struct (Project Settings > Zombie Survival > Item Override Table) adds to or overrides it.
 */
USTRUCT(BlueprintType)
struct FZSItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EZSItemCategory Category = EZSItemCategory::Junk;

	/** Reference price in dollars. Merchants apply their own markup / buy rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BaseValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxStack = 1;

	/** Swings (melee) or shots (firearm) before the item breaks. 0 = never wears out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxDurability = 0;

	/** Damage per hit / per pellet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 0.f;

	/** Reach for melee, max distance for firearms (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Range = 0.f;

	/** Seconds between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackInterval = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StaminaCost = 0.f;

	/** How far (cm) zombies can hear one use of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Noise = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ReloadTime = 0.f;

	/** Item id of the ammo this firearm consumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AmmoItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Pellets = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpreadDegrees = 0.f;

	/** Health restored when used (Medical / Food). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HealAmount = 0.f;

	bool IsWeapon() const { return Category == EZSItemCategory::Melee || Category == EZSItemCategory::Firearm; }
	bool IsFirearm() const { return Category == EZSItemCategory::Firearm; }
};

/** A slot of an inventory: an item id plus its per-instance state. */
USTRUCT(BlueprintType)
struct FZSItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	/** Remaining uses for durable items (<= 0 means "fill to max" when added). Not tracked on stackable items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Durability = 0;

	/** Rounds currently loaded (firearms only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AmmoInMag = 0;

	FZSItemStack() {}
	FZSItemStack(FName InItemId, int32 InQuantity, int32 InDurability = 0, int32 InAmmoInMag = 0)
		: ItemId(InItemId), Quantity(InQuantity), Durability(InDurability), AmmoInMag(InAmmoInMag)
	{
	}

	bool IsEmpty() const { return ItemId.IsNone() || Quantity <= 0; }
};
