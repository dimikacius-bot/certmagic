#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ZSItemTypes.h"
#include "Inventory/ZSInventoryComponent.h"
#include "ZSMerchantComponent.generated.h"

UENUM(BlueprintType)
enum class EZSTradeResult : uint8
{
	Success,
	InvalidItem,
	OutOfStock,
	NotInterested,
	NotEnoughMoney,
	MerchantNoCash,
	InventoryFull
};

USTRUCT(BlueprintType)
struct FZSMerchantStockEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;

	FZSMerchantStockEntry() {}
	FZSMerchantStockEntry(FName InItemId, int32 InQuantity) : ItemId(InItemId), Quantity(InQuantity) {}
};

/**
 * A trader. Owns pricing rules and the trade transactions; its stock room and cash drawer are a
 * sibling UZSInventoryComponent (created on demand).
 *
 * Economy rules
 *  - Player buys at  BaseValue x SellMarkup x RegionPriceModifier x condition (+15% when nearly out).
 *  - Player sells at BaseValue x BuyRate x RegionPriceModifier x condition x demand, where demand
 *    falls the more of that item the merchant already holds.
 *  - A merchant can only pay what is in its drawer, and it will only buy what it is interested in.
 *  - Everything the player sells goes into the merchant's stock and can be bought back (at markup).
 *  - Every in-game day the drawer refills a little and the default stock is replenished.
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class ZOMBIESURVIVAL_API UZSMerchantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZSMerchantComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	FText MerchantName = FText::FromString(TEXT("Trader"));

	/** Multiplier on BaseValue when the player buys. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant", meta = (ClampMin = "0.5"))
	float SellMarkup = 1.3f;

	/** Fraction of BaseValue paid when the player sells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float BuyRate = 0.4f;

	/** Local flavour: > 1 in remote/dangerous places, < 1 in trade hubs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant", meta = (ClampMin = "0.25"))
	float RegionPriceModifier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	int32 StartingCash = 600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	int32 MaxCash = 1500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	int32 DailyCashRegen = 250;

	/** Categories this merchant buys. Empty = anything. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	TArray<EZSItemCategory> BuysCategories;

	/** Items the merchant restocks every day (up to this quantity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Merchant")
	TArray<FZSMerchantStockEntry> DefaultStock;

	UFUNCTION(BlueprintPure, Category = "ZS|Merchant")
	UZSInventoryComponent* GetStock() const { return Stock; }

	UFUNCTION(BlueprintPure, Category = "ZS|Merchant")
	bool WillBuy(FName ItemId) const;

	/** Price the player pays for ONE unit of this stack. */
	UFUNCTION(BlueprintPure, Category = "ZS|Merchant")
	int32 GetBuyPrice(const FZSItemStack& Stack) const;

	/** Total the merchant pays for Quantity units of this stack (0 when not interested). */
	UFUNCTION(BlueprintPure, Category = "ZS|Merchant")
	int32 GetSellPrice(const FZSItemStack& Stack, int32 Quantity) const;

	/** The player buys Quantity from merchant stock slot StockSlot. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Merchant")
	EZSTradeResult TryBuy(UZSInventoryComponent* Buyer, int32 StockSlot, int32 Quantity, int32& OutPrice);

	/** The player sells Quantity from their inventory slot SellerSlot. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Merchant")
	EZSTradeResult TrySell(UZSInventoryComponent* Seller, int32 SellerSlot, int32 Quantity, int32& OutPrice);

	UFUNCTION(BlueprintCallable, Category = "ZS|Merchant")
	void DailyRestock();

	static FText DescribeResult(EZSTradeResult Result);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleNewDay(int32 DayNumber);

	UPROPERTY()
	TObjectPtr<UZSInventoryComponent> Stock;
};
