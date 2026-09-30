#include "Economy/ZSLootTables.h"
#include "Inventory/ZSItemSubsystem.h"

namespace
{
	struct FLootEntry
	{
		const TCHAR* ItemId;
		float Weight;
		int32 MinQty;
		int32 MaxQty;
	};

	struct FLootTableDef
	{
		int32 MinRolls;
		int32 MaxRolls;
		float CashChance;
		int32 CashMin;
		int32 CashMax;
		TArray<FLootEntry> Entries;
	};

	const FLootTableDef& GetDef(EZSLootTable Table)
	{
		static const FLootTableDef Residential = { 1, 3, 0.45f, 2, 25, {
			{ TEXT("Bandage"), 10.f, 1, 2 }, { TEXT("CannedFood"), 12.f, 1, 2 }, { TEXT("EnergyBar"), 8.f, 1, 3 },
			{ TEXT("Scrap"), 12.f, 2, 6 }, { TEXT("Cloth"), 10.f, 2, 5 }, { TEXT("Watch"), 5.f, 1, 1 },
			{ TEXT("Jewelry"), 2.f, 1, 1 }, { TEXT("Electronics"), 5.f, 1, 2 }, { TEXT("Pipe"), 3.f, 1, 1 },
			{ TEXT("BaseballBat"), 2.f, 1, 1 }, { TEXT("Ammo9mm"), 3.f, 4, 12 }, { TEXT("Medkit"), 1.f, 1, 1 } } };

		static const FLootTableDef Medical = { 1, 3, 0.15f, 5, 30, {
			{ TEXT("Bandage"), 25.f, 1, 3 }, { TEXT("Medkit"), 10.f, 1, 1 }, { TEXT("Antibiotics"), 8.f, 1, 2 },
			{ TEXT("CannedFood"), 3.f, 1, 1 } } };

		static const FLootTableDef Police = { 1, 2, 0.5f, 10, 60, {
			{ TEXT("Pistol9mm"), 6.f, 1, 1 }, { TEXT("Ammo9mm"), 22.f, 6, 18 }, { TEXT("Bandage"), 8.f, 1, 2 },
			{ TEXT("BaseballBat"), 3.f, 1, 1 }, { TEXT("AmmoShell"), 4.f, 3, 8 }, { TEXT("Shotgun"), 1.5f, 1, 1 } } };

		static const FLootTableDef Military = { 1, 3, 0.4f, 20, 120, {
			{ TEXT("Ammo9mm"), 15.f, 8, 24 }, { TEXT("AmmoRifle"), 12.f, 5, 15 }, { TEXT("AmmoShell"), 8.f, 4, 10 },
			{ TEXT("HuntingRifle"), 3.f, 1, 1 }, { TEXT("Shotgun"), 3.f, 1, 1 }, { TEXT("Pistol9mm"), 5.f, 1, 1 },
			{ TEXT("Medkit"), 8.f, 1, 1 }, { TEXT("Bandage"), 10.f, 1, 3 }, { TEXT("FireAxe"), 1.f, 1, 1 } } };

		static const FLootTableDef Industrial = { 2, 4, 0.2f, 5, 30, {
			{ TEXT("Scrap"), 25.f, 3, 10 }, { TEXT("DuctTape"), 8.f, 1, 3 }, { TEXT("Electronics"), 10.f, 1, 3 },
			{ TEXT("Pipe"), 8.f, 1, 1 }, { TEXT("FireAxe"), 3.f, 1, 1 }, { TEXT("Machete"), 2.f, 1, 1 },
			{ TEXT("Cloth"), 6.f, 2, 6 } } };

		static const FLootTableDef Zombie = { 0, 1, 0.35f, 1, 9, {
			{ TEXT("Scrap"), 5.f, 1, 2 }, { TEXT("Cloth"), 4.f, 1, 2 }, { TEXT("Ammo9mm"), 2.f, 1, 3 },
			{ TEXT("Bandage"), 1.f, 1, 1 }, { TEXT("Watch"), 1.f, 1, 1 }, { TEXT("Jewelry"), 0.3f, 1, 1 } } };

		switch (Table)
		{
		case EZSLootTable::Medical:    return Medical;
		case EZSLootTable::Police:     return Police;
		case EZSLootTable::Military:   return Military;
		case EZSLootTable::Industrial: return Industrial;
		case EZSLootTable::Zombie:     return Zombie;
		default:                       return Residential;
		}
	}
}

FZSLootResult ZSLoot::Roll(const UObject* WorldContext, EZSLootTable Table)
{
	FZSLootResult Result;
	const FLootTableDef& Def = GetDef(Table);
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(WorldContext);

	float TotalWeight = 0.f;
	for (const FLootEntry& Entry : Def.Entries)
	{
		TotalWeight += Entry.Weight;
	}

	const int32 Rolls = FMath::RandRange(Def.MinRolls, Def.MaxRolls);
	for (int32 Index = 0; Index < Rolls && TotalWeight > 0.f; ++Index)
	{
		float Pick = FMath::FRand() * TotalWeight;
		for (const FLootEntry& Entry : Def.Entries)
		{
			Pick -= Entry.Weight;
			if (Pick > 0.f)
			{
				continue;
			}

			FZSItemStack Stack(FName(Entry.ItemId), FMath::RandRange(Entry.MinQty, Entry.MaxQty));

			// Found weapons are already worn.
			const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
			if (Row && Row->MaxDurability > 0)
			{
				Stack.Durability = FMath::Max(1, FMath::RoundToInt(Row->MaxDurability * FMath::FRandRange(0.35f, 1.f)));
			}

			Result.Items.Add(Stack);
			break;
		}
	}

	if (FMath::FRand() < Def.CashChance)
	{
		Result.Cash = FMath::RandRange(Def.CashMin, Def.CashMax);
	}
	return Result;
}
