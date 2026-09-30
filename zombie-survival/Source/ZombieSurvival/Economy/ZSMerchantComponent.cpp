#include "Economy/ZSMerchantComponent.h"
#include "Core/ZSWorldDirector.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"

namespace
{
	/** Worn weapons are worth less: 35% of full price at 0 durability, 100% when brand new. */
	float ConditionFactor(const FZSItemRow& Row, const FZSItemStack& Stack)
	{
		if (Row.MaxDurability <= 0)
		{
			return 1.f;
		}
		const float Ratio = FMath::Clamp(static_cast<float>(Stack.Durability) / Row.MaxDurability, 0.f, 1.f);
		return FMath::Lerp(0.35f, 1.f, Ratio);
	}
}

UZSMerchantComponent::UZSMerchantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZSMerchantComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	Stock = Owner ? Owner->FindComponentByClass<UZSInventoryComponent>() : nullptr;
	if (!Stock && Owner)
	{
		Stock = NewObject<UZSInventoryComponent>(Owner, TEXT("MerchantStock"));
		Stock->RegisterComponent();
	}
	if (!Stock)
	{
		return;
	}

	Stock->MaxSlots = 64;
	Stock->SetMoney(StartingCash);
	for (const FZSMerchantStockEntry& Entry : DefaultStock)
	{
		Stock->AddItem(Entry.ItemId, Entry.Quantity);
	}

	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->OnNewDay.AddDynamic(this, &UZSMerchantComponent::HandleNewDay);
	}
}

void UZSMerchantComponent::HandleNewDay(int32 DayNumber)
{
	DailyRestock();
}

void UZSMerchantComponent::DailyRestock()
{
	if (!Stock)
	{
		return;
	}

	Stock->SetMoney(FMath::Min(MaxCash, Stock->GetMoney() + DailyCashRegen));

	// Things the player sold us that are not part of our usual range get bought by other customers.
	TSet<FName> DefaultIds;
	for (const FZSMerchantStockEntry& Entry : DefaultStock)
	{
		DefaultIds.Add(Entry.ItemId);
	}
	TArray<FZSItemStack>& Slots = Stock->GetMutableSlots();
	for (int32 Index = Slots.Num() - 1; Index >= 0; --Index)
	{
		if (!DefaultIds.Contains(Slots[Index].ItemId) && FMath::FRand() < 0.4f)
		{
			Slots.RemoveAt(Index);
		}
	}

	for (const FZSMerchantStockEntry& Entry : DefaultStock)
	{
		const int32 Missing = Entry.Quantity - Stock->CountItem(Entry.ItemId);
		if (Missing > 0)
		{
			Stock->AddItem(Entry.ItemId, Missing);
		}
	}
}

bool UZSMerchantComponent::WillBuy(FName ItemId) const
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FZSItemRow* Row = Items ? Items->Find(ItemId) : nullptr;
	if (!Row)
	{
		return false;
	}
	return BuysCategories.Num() == 0 || BuysCategories.Contains(Row->Category);
}

int32 UZSMerchantComponent::GetBuyPrice(const FZSItemStack& Stack) const
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
	if (!Row)
	{
		return 0;
	}

	float Price = Row->BaseValue * SellMarkup * RegionPriceModifier * ConditionFactor(*Row, Stack);
	if (Stock && Stock->CountItem(Stack.ItemId) <= 2)
	{
		Price *= 1.15f; // scarcity
	}
	return FMath::Max(1, FMath::RoundToInt(Price));
}

int32 UZSMerchantComponent::GetSellPrice(const FZSItemStack& Stack, int32 Quantity) const
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
	if (!Row || !WillBuy(Stack.ItemId) || Quantity <= 0)
	{
		return 0;
	}

	const int32 Held = Stock ? Stock->CountItem(Stack.ItemId) : 0;
	const float Unit = Row->BaseValue * BuyRate * RegionPriceModifier * ConditionFactor(*Row, Stack);

	float Total = 0.f;
	for (int32 Index = 0; Index < Quantity; ++Index)
	{
		// The more the merchant already holds, the less it wants another.
		const float Demand = FMath::Clamp(1.3f - 0.05f * (Held + Index), 0.4f, 1.3f);
		Total += Unit * Demand;
	}
	return FMath::Max(1, FMath::RoundToInt(Total));
}

EZSTradeResult UZSMerchantComponent::TryBuy(UZSInventoryComponent* Buyer, int32 StockSlot, int32 Quantity, int32& OutPrice)
{
	OutPrice = 0;
	if (!Stock || !Buyer || Quantity <= 0 || !Stock->GetSlots().IsValidIndex(StockSlot))
	{
		return EZSTradeResult::InvalidItem;
	}

	const FZSItemStack Offer = Stock->GetSlots()[StockSlot];
	Quantity = FMath::Min(Quantity, Offer.Quantity);
	if (Quantity <= 0)
	{
		return EZSTradeResult::OutOfStock;
	}

	const int32 Total = GetBuyPrice(Offer) * Quantity;
	if (Buyer->GetMoney() < Total)
	{
		return EZSTradeResult::NotEnoughMoney;
	}

	FZSItemStack Probe = Offer;
	Probe.Quantity = Quantity;
	if (!Buyer->CanFit(Probe))
	{
		return EZSTradeResult::InventoryFull;
	}

	FZSItemStack Moved;
	Stock->RemoveFromSlot(StockSlot, Quantity, Moved);
	Buyer->AddStack(Moved);
	Buyer->TrySpendMoney(Total);
	Stock->AddMoney(Total);

	OutPrice = Total;
	return EZSTradeResult::Success;
}

EZSTradeResult UZSMerchantComponent::TrySell(UZSInventoryComponent* Seller, int32 SellerSlot, int32 Quantity, int32& OutPrice)
{
	OutPrice = 0;
	if (!Stock || !Seller || Quantity <= 0 || !Seller->GetSlots().IsValidIndex(SellerSlot))
	{
		return EZSTradeResult::InvalidItem;
	}

	const FZSItemStack Offer = Seller->GetSlots()[SellerSlot];
	Quantity = FMath::Min(Quantity, Offer.Quantity);

	if (!WillBuy(Offer.ItemId))
	{
		return EZSTradeResult::NotInterested;
	}

	const int32 Total = GetSellPrice(Offer, Quantity);
	if (Stock->GetMoney() < Total)
	{
		return EZSTradeResult::MerchantNoCash;
	}

	FZSItemStack Probe = Offer;
	Probe.Quantity = Quantity;
	if (!Stock->CanFit(Probe))
	{
		return EZSTradeResult::InventoryFull;
	}

	FZSItemStack Moved;
	Seller->RemoveFromSlot(SellerSlot, Quantity, Moved);
	Stock->AddStack(Moved);
	Stock->TrySpendMoney(Total);
	Seller->AddMoney(Total);

	OutPrice = Total;
	return EZSTradeResult::Success;
}

FText UZSMerchantComponent::DescribeResult(EZSTradeResult Result)
{
	switch (Result)
	{
	case EZSTradeResult::Success:        return FText::FromString(TEXT("Deal."));
	case EZSTradeResult::InvalidItem:    return FText::FromString(TEXT("Nothing there."));
	case EZSTradeResult::OutOfStock:     return FText::FromString(TEXT("Sold out."));
	case EZSTradeResult::NotInterested:  return FText::FromString(TEXT("Not interested in that."));
	case EZSTradeResult::NotEnoughMoney: return FText::FromString(TEXT("You can't afford that."));
	case EZSTradeResult::MerchantNoCash: return FText::FromString(TEXT("The trader can't pay that much right now."));
	case EZSTradeResult::InventoryFull:  return FText::FromString(TEXT("No room."));
	}
	return FText::GetEmpty();
}
