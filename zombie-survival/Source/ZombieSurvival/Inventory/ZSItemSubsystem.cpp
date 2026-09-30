#include "Inventory/ZSItemSubsystem.h"
#include "Core/ZSSettings.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UZSItemSubsystem* UZSItemSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UZSItemSubsystem>() : nullptr;
}

void UZSItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RegisterBuiltins();

	if (UDataTable* Override = UZSSettings::Get()->ItemOverrideTable.LoadSynchronous())
	{
		Override->ForeachRow<FZSItemRow>(TEXT("ZSItemOverride"), [this](const FName& Key, const FZSItemRow& Row)
		{
			Items.Add(Key, Row);
		});
	}
}

FText UZSItemSubsystem::GetDisplayName(FName ItemId) const
{
	if (const FZSItemRow* Row = Find(ItemId))
	{
		return Row->DisplayName;
	}
	return FText::FromName(ItemId);
}

namespace
{
	FZSItemRow& Define(TMap<FName, FZSItemRow>& Map, const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
		EZSItemCategory Category, int32 Value, int32 MaxStack)
	{
		FZSItemRow Row;
		Row.DisplayName = FText::FromString(Name);
		Row.Description = FText::FromString(Description);
		Row.Category = Category;
		Row.BaseValue = Value;
		Row.MaxStack = MaxStack;
		return Map.Add(FName(Id), Row);
	}
}

void UZSItemSubsystem::RegisterBuiltins()
{
	Items.Reset();
	using Cat = EZSItemCategory;

	// --- Melee ------------------------------------------------------------------------------
	{
		FZSItemRow& R = Define(Items, TEXT("Pipe"), TEXT("Lead Pipe"), TEXT("Heavy, quiet, and everywhere."), Cat::Melee, 40, 1);
		R.Damage = 34.f; R.Range = 170.f; R.AttackInterval = 0.65f; R.StaminaCost = 9.f; R.MaxDurability = 60; R.Noise = 700.f;
	}
	{
		FZSItemRow& R = Define(Items, TEXT("BaseballBat"), TEXT("Baseball Bat"), TEXT("Good reach, good swing."), Cat::Melee, 65, 1);
		R.Damage = 42.f; R.Range = 180.f; R.AttackInterval = 0.75f; R.StaminaCost = 11.f; R.MaxDurability = 55; R.Noise = 800.f;
	}
	{
		FZSItemRow& R = Define(Items, TEXT("Machete"), TEXT("Machete"), TEXT("Fast and deadly. Lasts a while."), Cat::Melee, 160, 1);
		R.Damage = 58.f; R.Range = 165.f; R.AttackInterval = 0.6f; R.StaminaCost = 10.f; R.MaxDurability = 80; R.Noise = 600.f;
	}
	{
		FZSItemRow& R = Define(Items, TEXT("FireAxe"), TEXT("Fire Axe"), TEXT("Slow, brutal, crowd-clearing."), Cat::Melee, 230, 1);
		R.Damage = 85.f; R.Range = 175.f; R.AttackInterval = 1.05f; R.StaminaCost = 18.f; R.MaxDurability = 70; R.Noise = 900.f;
	}

	// --- Firearms (loud: they call every zombie in earshot) -----------------------------------
	{
		FZSItemRow& R = Define(Items, TEXT("Pistol9mm"), TEXT("9mm Pistol"), TEXT("Reliable sidearm."), Cat::Firearm, 380, 1);
		R.Damage = 30.f; R.Range = 7000.f; R.AttackInterval = 0.32f; R.MaxDurability = 240; R.Noise = 4500.f;
		R.MagazineSize = 12; R.ReloadTime = 1.6f; R.AmmoItem = TEXT("Ammo9mm"); R.SpreadDegrees = 1.2f;
	}
	{
		FZSItemRow& R = Define(Items, TEXT("Shotgun"), TEXT("Pump Shotgun"), TEXT("Devastating up close."), Cat::Firearm, 950, 1);
		R.Damage = 14.f; R.Range = 2200.f; R.AttackInterval = 0.95f; R.MaxDurability = 120; R.Noise = 5500.f;
		R.MagazineSize = 6; R.ReloadTime = 2.8f; R.AmmoItem = TEXT("AmmoShell"); R.SpreadDegrees = 6.f; R.Pellets = 8;
	}
	{
		FZSItemRow& R = Define(Items, TEXT("HuntingRifle"), TEXT("Hunting Rifle"), TEXT("One shot, one zombie."), Cat::Firearm, 1500, 1);
		R.Damage = 115.f; R.Range = 20000.f; R.AttackInterval = 1.25f; R.MaxDurability = 150; R.Noise = 6500.f;
		R.MagazineSize = 5; R.ReloadTime = 3.f; R.AmmoItem = TEXT("AmmoRifle"); R.SpreadDegrees = 0.15f;
	}

	// --- Ammo ---------------------------------------------------------------------------------
	Define(Items, TEXT("Ammo9mm"), TEXT("9mm Rounds"), TEXT("Pistol ammunition."), Cat::Ammo, 2, 60);
	Define(Items, TEXT("AmmoShell"), TEXT("12ga Shells"), TEXT("Shotgun ammunition."), Cat::Ammo, 5, 24);
	Define(Items, TEXT("AmmoRifle"), TEXT("Rifle Rounds"), TEXT("Hunting rifle ammunition."), Cat::Ammo, 7, 30);

	// --- Medical / food -----------------------------------------------------------------------
	Define(Items, TEXT("Bandage"), TEXT("Bandage"), TEXT("Stops the bleeding. Heals 25."), Cat::Medical, 18, 10).HealAmount = 25.f;
	Define(Items, TEXT("Medkit"), TEXT("Medkit"), TEXT("Proper first aid. Heals 75."), Cat::Medical, 70, 5).HealAmount = 75.f;
	Define(Items, TEXT("Antibiotics"), TEXT("Antibiotics"), TEXT("Worth their weight in gold."), Cat::Medical, 90, 5).HealAmount = 15.f;
	Define(Items, TEXT("CannedFood"), TEXT("Canned Food"), TEXT("Heals 8."), Cat::Food, 9, 10).HealAmount = 8.f;
	Define(Items, TEXT("EnergyBar"), TEXT("Energy Bar"), TEXT("Heals 5."), Cat::Food, 6, 10).HealAmount = 5.f;

	// --- Materials / trade goods --------------------------------------------------------------
	Define(Items, TEXT("Scrap"), TEXT("Scrap Metal"), TEXT("Sells by the pound."), Cat::Material, 3, 50);
	Define(Items, TEXT("Cloth"), TEXT("Cloth"), TEXT("Rags and fabric."), Cat::Material, 2, 30);
	Define(Items, TEXT("DuctTape"), TEXT("Duct Tape"), TEXT("Fixes everything."), Cat::Material, 12, 10);
	Define(Items, TEXT("Watch"), TEXT("Wristwatch"), TEXT("Still ticking."), Cat::Junk, 45, 5);
	Define(Items, TEXT("Jewelry"), TEXT("Jewelry"), TEXT("Someone's heirloom."), Cat::Junk, 130, 5);
	Define(Items, TEXT("Electronics"), TEXT("Electronics"), TEXT("Salvageable circuit boards."), Cat::Junk, 35, 10);
	Define(Items, TEXT("Radio"), TEXT("Hand Radio"), TEXT("Static and hope."), Cat::Junk, 80, 3);
}
