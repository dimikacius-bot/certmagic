#include "Player/ZSPlayerController.h"
#include "Economy/ZSMerchantComponent.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Player/ZSCharacter.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"

AZSPlayerController::AZSPlayerController()
{
}

void AZSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("MenuUp", IE_Pressed, this, &AZSPlayerController::MenuUp);
	InputComponent->BindAction("MenuDown", IE_Pressed, this, &AZSPlayerController::MenuDown);
	InputComponent->BindAction("MenuConfirm", IE_Pressed, this, &AZSPlayerController::MenuConfirm);
	InputComponent->BindAction("MenuTab", IE_Pressed, this, &AZSPlayerController::MenuTab);
	InputComponent->BindAction("MenuBack", IE_Pressed, this, &AZSPlayerController::MenuBack);
}

void AZSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	const float Now = GetWorld()->GetTimeSeconds();
	Notifications.RemoveAll([Now](const FZSNotification& N) { return N.ExpireTime <= Now; });

	// Walking away (or dying) ends the trade.
	if (bTrading)
	{
		const AActor* Merchant = TradeMerchantActor.Get();
		const AZSCharacter* Player = Cast<AZSCharacter>(GetPawn());
		if (!Merchant || !Player || Player->IsDead() || FVector::DistSquared(Merchant->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(450.f))
		{
			CloseTrade();
		}
	}
}

// --- Trading ----------------------------------------------------------------------------------

UZSMerchantComponent* AZSPlayerController::GetTradeMerchant() const
{
	const AActor* Actor = TradeMerchantActor.Get();
	return Actor ? Actor->FindComponentByClass<UZSMerchantComponent>() : nullptr;
}

void AZSPlayerController::OpenTrade(AActor* MerchantActor)
{
	if (bTrading || !MerchantActor || !MerchantActor->FindComponentByClass<UZSMerchantComponent>())
	{
		return;
	}

	TradeMerchantActor = MerchantActor;
	bTrading = true;
	bSellColumn = false;
	TradeCursor = 0;
	TradeMessage = TEXT("Enter: trade one   Shift+Enter: trade five   Tab: switch column   E: leave");
	TradeMessageColor = FLinearColor(0.8f, 0.8f, 0.8f);
	bInventoryOpen = false;

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AZSPlayerController::CloseTrade()
{
	if (!bTrading)
	{
		return;
	}

	bTrading = false;
	TradeMerchantActor.Reset();
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
}

int32 AZSPlayerController::GetActiveListNum() const
{
	if (bSellColumn)
	{
		const AZSCharacter* Player = Cast<AZSCharacter>(GetPawn());
		return Player ? Player->GetInventory()->GetSlots().Num() : 0;
	}

	const UZSMerchantComponent* Merchant = GetTradeMerchant();
	return (Merchant && Merchant->GetStock()) ? Merchant->GetStock()->GetSlots().Num() : 0;
}

void AZSPlayerController::ClampCursor()
{
	TradeCursor = FMath::Clamp(TradeCursor, 0, FMath::Max(0, GetActiveListNum() - 1));
}

void AZSPlayerController::MenuUp()
{
	if (bTrading)
	{
		TradeCursor = FMath::Max(0, TradeCursor - 1);
	}
}

void AZSPlayerController::MenuDown()
{
	if (bTrading)
	{
		TradeCursor = FMath::Min(TradeCursor + 1, FMath::Max(0, GetActiveListNum() - 1));
	}
}

void AZSPlayerController::MenuTab()
{
	if (bTrading)
	{
		bSellColumn = !bSellColumn;
		TradeCursor = 0;
	}
}

void AZSPlayerController::MenuBack()
{
	CloseTrade();
	bInventoryOpen = false;
}

void AZSPlayerController::MenuConfirm()
{
	if (!bTrading)
	{
		return;
	}

	UZSMerchantComponent* Merchant = GetTradeMerchant();
	AZSCharacter* Player = Cast<AZSCharacter>(GetPawn());
	if (!Merchant || !Player)
	{
		return;
	}

	const int32 Quantity = IsInputKeyDown(EKeys::LeftShift) ? 5 : 1;
	int32 Price = 0;
	const EZSTradeResult Result = bSellColumn
		? Merchant->TrySell(Player->GetInventory(), TradeCursor, Quantity, Price)
		: Merchant->TryBuy(Player->GetInventory(), TradeCursor, Quantity, Price);

	if (Result == EZSTradeResult::Success)
	{
		TradeMessage = FString::Printf(bSellColumn ? TEXT("Sold for $%d") : TEXT("Bought for $%d"), Price);
		TradeMessageColor = FLinearColor(0.4f, 1.f, 0.4f);
	}
	else
	{
		TradeMessage = UZSMerchantComponent::DescribeResult(Result).ToString();
		TradeMessageColor = FLinearColor(1.f, 0.5f, 0.3f);
	}

	ClampCursor();
}

// --- Messages ---------------------------------------------------------------------------------

void AZSPlayerController::PushNotification(const FText& Message, const FLinearColor& Color)
{
	FZSNotification Entry;
	Entry.Text = Message.ToString();
	Entry.Color = Color;
	Entry.ExpireTime = GetWorld()->GetTimeSeconds() + 4.f;
	Notifications.Add(Entry);

	if (Notifications.Num() > 6)
	{
		Notifications.RemoveAt(0);
	}
}
