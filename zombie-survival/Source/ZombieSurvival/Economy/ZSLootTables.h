#pragma once

#include "CoreMinimal.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSLootTables.generated.h"

UENUM(BlueprintType)
enum class EZSLootTable : uint8
{
	Residential,
	Medical,
	Police,
	Military,
	Industrial,
	Zombie
};

/** What one roll of a loot table produced. */
struct FZSLootResult
{
	TArray<FZSItemStack> Items;
	int32 Cash = 0;
};

namespace ZSLoot
{
	/** Rolls a table: some weighted items (weapons come pre-worn) plus a chance of loose cash. */
	FZSLootResult Roll(const UObject* WorldContext, EZSLootTable Table);
}
