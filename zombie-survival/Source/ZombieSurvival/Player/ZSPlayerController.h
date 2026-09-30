#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZSPlayerController.generated.h"

class UZSMerchantComponent;

struct FZSNotification
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float ExpireTime = 0.f;
};

/**
 * Owns the UI state that survives the pawn: the trade session with a merchant, the inventory panel
 * and the message feed. Drawing is done by AZSHUD; keys are the MenuXxx actions in DefaultInput.ini.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZSPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	// --- Trading ------------------------------------------------------------------------------

	void OpenTrade(AActor* MerchantActor);
	void CloseTrade();

	bool IsTrading() const { return bTrading; }
	UZSMerchantComponent* GetTradeMerchant() const;

	/** false = the BUY column (merchant stock) is active, true = the SELL column (your inventory). */
	bool IsSellColumn() const { return bSellColumn; }
	int32 GetTradeCursor() const { return TradeCursor; }
	const FString& GetTradeMessage() const { return TradeMessage; }
	FLinearColor GetTradeMessageColor() const { return TradeMessageColor; }

	// --- Inventory panel / messages -----------------------------------------------------------

	void ToggleInventory() { bInventoryOpen = !bInventoryOpen; }
	bool IsInventoryOpen() const { return bInventoryOpen; }

	void PushNotification(const FText& Message, const FLinearColor& Color);
	const TArray<FZSNotification>& GetNotifications() const { return Notifications; }

protected:
	virtual void SetupInputComponent() override;

private:
	void MenuUp();
	void MenuDown();
	void MenuConfirm();
	void MenuTab();
	void MenuBack();

	int32 GetActiveListNum() const;
	void ClampCursor();

	TWeakObjectPtr<AActor> TradeMerchantActor;
	bool bTrading = false;
	bool bSellColumn = false;
	int32 TradeCursor = 0;
	FString TradeMessage;
	FLinearColor TradeMessageColor = FLinearColor::White;

	bool bInventoryOpen = false;
	TArray<FZSNotification> Notifications;
};
