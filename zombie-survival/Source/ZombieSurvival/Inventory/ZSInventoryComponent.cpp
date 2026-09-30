#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"

UZSInventoryComponent::UZSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UZSInventoryComponent::AddItem(FName ItemId, int32 Quantity, int32 Durability, int32 AmmoInMag)
{
	return AddStack(FZSItemStack(ItemId, Quantity, Durability, AmmoInMag));
}

int32 UZSInventoryComponent::AddStack(const FZSItemStack& Stack)
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
	if (!Row || Stack.IsEmpty())
	{
		return FMath::Max(0, Stack.Quantity);
	}

	int32 Remaining = Stack.Quantity;

	if (Row->MaxStack > 1)
	{
		// Top up existing stacks first, then open new slots.
		for (FZSItemStack& Slot : Slots)
		{
			if (Remaining <= 0)
			{
				break;
			}
			if (Slot.ItemId == Stack.ItemId && Slot.Quantity < Row->MaxStack)
			{
				const int32 Moved = FMath::Min(Row->MaxStack - Slot.Quantity, Remaining);
				Slot.Quantity += Moved;
				Remaining -= Moved;
			}
		}
		while (Remaining > 0 && Slots.Num() < MaxSlots)
		{
			const int32 Moved = FMath::Min(Row->MaxStack, Remaining);
			Slots.Add(FZSItemStack(Stack.ItemId, Moved));
			Remaining -= Moved;
		}
	}
	else
	{
		// Non-stackable items keep their own durability / magazine.
		const int32 Durability = (Row->MaxDurability > 0 && Stack.Durability <= 0) ? Row->MaxDurability : Stack.Durability;
		while (Remaining > 0 && Slots.Num() < MaxSlots)
		{
			Slots.Add(FZSItemStack(Stack.ItemId, 1, Durability, Stack.AmmoInMag));
			--Remaining;
		}
	}

	if (Remaining != Stack.Quantity)
	{
		OnChanged.Broadcast();
	}
	return Remaining;
}

int32 UZSInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	int32 Removed = 0;
	for (int32 Index = Slots.Num() - 1; Index >= 0 && Removed < Quantity; --Index)
	{
		FZSItemStack& Slot = Slots[Index];
		if (Slot.ItemId != ItemId)
		{
			continue;
		}
		const int32 Taken = FMath::Min(Slot.Quantity, Quantity - Removed);
		Slot.Quantity -= Taken;
		Removed += Taken;
		if (Slot.Quantity <= 0)
		{
			Slots.RemoveAt(Index);
		}
	}

	if (Removed > 0)
	{
		OnChanged.Broadcast();
	}
	return Removed;
}

bool UZSInventoryComponent::RemoveFromSlot(int32 SlotIndex, int32 Quantity, FZSItemStack& OutRemoved)
{
	if (!Slots.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FZSItemStack& Slot = Slots[SlotIndex];
	const int32 Taken = FMath::Min(Slot.Quantity, Quantity);

	OutRemoved = Slot;
	OutRemoved.Quantity = Taken;

	Slot.Quantity -= Taken;
	if (Slot.Quantity <= 0)
	{
		Slots.RemoveAt(SlotIndex);
	}

	OnChanged.Broadcast();
	return true;
}

int32 UZSInventoryComponent::CountItem(FName ItemId) const
{
	int32 Count = 0;
	for (const FZSItemStack& Slot : Slots)
	{
		if (Slot.ItemId == ItemId)
		{
			Count += Slot.Quantity;
		}
	}
	return Count;
}

bool UZSInventoryComponent::CanFit(const FZSItemStack& Stack) const
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
	if (!Row || Stack.IsEmpty())
	{
		return false;
	}

	const int32 FreeSlots = FMath::Max(0, MaxSlots - Slots.Num());
	if (Row->MaxStack <= 1)
	{
		return FreeSlots >= Stack.Quantity;
	}

	int32 Room = FreeSlots * Row->MaxStack;
	for (const FZSItemStack& Slot : Slots)
	{
		if (Slot.ItemId == Stack.ItemId)
		{
			Room += FMath::Max(0, Row->MaxStack - Slot.Quantity);
		}
	}
	return Room >= Stack.Quantity;
}

void UZSInventoryComponent::SetMoney(int32 NewMoney)
{
	Money = FMath::Max(0, NewMoney);
	OnChanged.Broadcast();
}

void UZSInventoryComponent::AddMoney(int32 Amount)
{
	if (Amount > 0)
	{
		Money += Amount;
		OnChanged.Broadcast();
	}
}

bool UZSInventoryComponent::TrySpendMoney(int32 Amount)
{
	if (Amount < 0 || Money < Amount)
	{
		return false;
	}
	Money -= Amount;
	OnChanged.Broadcast();
	return true;
}
