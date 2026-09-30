#include "Player/ZSHUD.h"
#include "Core/ZSWorldDirector.h"
#include "Economy/ZSMerchantComponent.h"
#include "Economy/ZSSafeZone.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"
#include "Player/ZSCharacter.h"
#include "Player/ZSPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
	const FLinearColor kPanel(0.02f, 0.02f, 0.02f, 0.82f);
	const FLinearColor kText(0.92f, 0.92f, 0.92f, 1.f);
	const FLinearColor kDim(0.6f, 0.6f, 0.6f, 1.f);
	const FLinearColor kGold(1.f, 0.85f, 0.3f, 1.f);
	const FLinearColor kSelect(0.25f, 0.45f, 0.75f, 0.85f);
}

void AZSHUD::DrawHUD()
{
	Super::DrawHUD();

	const AZSPlayerController* Controller = Cast<AZSPlayerController>(GetOwningPlayerController());
	const AZSCharacter* Player = Controller ? Cast<AZSCharacter>(Controller->GetPawn()) : nullptr;
	if (!Player || !Canvas)
	{
		return;
	}

	UIScale = FMath::Clamp(Canvas->SizeY / 1080.f, 0.75f, 2.f);

	DrawOverlays(*Player);
	DrawVitals(*Player);
	DrawWeapon(*Player);
	DrawClockAndCash(*Player);
	DrawCrosshairAndPrompt(*Player, *Controller);
	DrawNotifications(*Controller);

	if (Controller->IsTrading())
	{
		DrawTrade(*Player, *Controller);
	}
	else if (Controller->IsInventoryOpen())
	{
		DrawInventory(*Player);
	}
}

// --- Helpers ----------------------------------------------------------------------------------

float AZSHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter, bool bRightAlign)
{
	UFont* Font = GEngine->GetMediumFont();
	const float FinalScale = Scale * UIScale;

	float Width = 0.f, Height = 0.f;
	GetTextSize(Text, Width, Height, Font, FinalScale);

	if (bCenter)
	{
		X -= Width * 0.5f;
	}
	else if (bRightAlign)
	{
		X -= Width;
	}

	// Cheap drop shadow keeps text readable on any background.
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 1.5f, Y + 1.5f, Font, FinalScale);
	DrawText(Text, Color, X, Y, Font, FinalScale);
	return Height;
}

void AZSHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f);
	DrawRect(Fill, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}

FString AZSHUD::DescribeStack(const FZSItemStack& Stack, const UZSItemSubsystem* Items) const
{
	const FZSItemRow* Row = Items ? Items->Find(Stack.ItemId) : nullptr;
	FString Name = Row ? Row->DisplayName.ToString() : Stack.ItemId.ToString();

	if (Stack.Quantity > 1)
	{
		Name += FString::Printf(TEXT(" x%d"), Stack.Quantity);
	}
	if (Row && Row->MaxDurability > 0)
	{
		Name += FString::Printf(TEXT("  [%d%%]"), FMath::RoundToInt(100.f * Stack.Durability / Row->MaxDurability));
	}
	if (Row && Row->IsFirearm())
	{
		Name += FString::Printf(TEXT("  (%d/%d)"), Stack.AmmoInMag, Row->MagazineSize);
	}
	return Name;
}

// --- Sections ---------------------------------------------------------------------------------

void AZSHUD::DrawOverlays(const AZSCharacter& Player)
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;

	// Red flash on damage, and a permanent warning tint when nearly dead.
	const float SinceDamage = Player.GetSecondsSinceDamage();
	if (SinceDamage < 0.6f)
	{
		DrawRect(FLinearColor(0.7f, 0.f, 0.f, (1.f - SinceDamage / 0.6f) * 0.35f), 0.f, 0.f, W, H);
	}

	const UZSHealthComponent* Health = Player.GetHealthComponent();
	const float Fraction = Health->GetHealth() / Health->GetMaxHealth();
	if (!Player.IsDead() && Fraction < 0.3f)
	{
		DrawRect(FLinearColor(0.5f, 0.f, 0.f, (0.3f - Fraction) * 0.6f), 0.f, 0.f, W, H);
	}

	if (Player.IsDead())
	{
		DrawRect(FLinearColor(0.25f, 0.f, 0.f, 0.6f), 0.f, 0.f, W, H);
		DrawLabel(TEXT("YOU DIED"), W * 0.5f, H * 0.42f, FLinearColor(0.9f, 0.1f, 0.1f), 3.f, true);
		DrawLabel(TEXT("Some of your cash is where you fell."), W * 0.5f, H * 0.42f + 70.f * UIScale, kText, 1.1f, true);
	}
}

void AZSHUD::DrawVitals(const AZSCharacter& Player)
{
	const float X = 40.f * UIScale;
	const float BarW = 300.f * UIScale;
	const float BarH = 16.f * UIScale;
	float Y = Canvas->SizeY - 100.f * UIScale;

	const UZSHealthComponent* Health = Player.GetHealthComponent();
	DrawLabel(FString::Printf(TEXT("HEALTH  %d / %d"), FMath::CeilToInt(Health->GetHealth()), FMath::RoundToInt(Health->GetMaxHealth())), X, Y - 22.f * UIScale, kText, 0.85f);
	DrawBar(X, Y, BarW, BarH, Health->GetHealth() / Health->GetMaxHealth(), FLinearColor(0.75f, 0.1f, 0.1f));

	Y += 44.f * UIScale;
	const FLinearColor StaminaColor = Player.IsExhausted() ? FLinearColor(0.7f, 0.35f, 0.1f) : FLinearColor(0.9f, 0.8f, 0.2f);
	DrawLabel(Player.IsExhausted() ? TEXT("STAMINA  (exhausted)") : TEXT("STAMINA"), X, Y - 22.f * UIScale, kText, 0.85f);
	DrawBar(X, Y, BarW, BarH * 0.7f, Player.GetStamina() / Player.GetMaxStamina(), StaminaColor);
}

void AZSHUD::DrawWeapon(const AZSCharacter& Player)
{
	const float Right = Canvas->SizeX - 40.f * UIScale;
	const float Y = Canvas->SizeY - 110.f * UIScale;

	const UZSCombatComponent* Combat = Player.GetCombat();
	const FZSItemStack* Stack = Combat->GetEquippedStack();
	const FZSItemRow* Row = Combat->GetEquippedRow();

	if (!Stack || !Row)
	{
		DrawLabel(TEXT("Fists"), Right, Y, kText, 1.2f, false, true);
		return;
	}

	DrawLabel(Row->DisplayName.ToString(), Right, Y, kText, 1.2f, false, true);

	if (Row->IsFirearm())
	{
		const int32 Reserve = Player.GetInventory()->CountItem(Row->AmmoItem);
		const FString Ammo = Combat->IsReloading()
			? FString(TEXT("RELOADING..."))
			: FString::Printf(TEXT("%d / %d"), Stack->AmmoInMag, Reserve);
		DrawLabel(Ammo, Right, Y + 34.f * UIScale, Stack->AmmoInMag == 0 ? FLinearColor(1.f, 0.4f, 0.3f) : kGold, 1.5f, false, true);
	}

	if (Row->MaxDurability > 0)
	{
		const float Fraction = static_cast<float>(Stack->Durability) / Row->MaxDurability;
		const float BarW = 160.f * UIScale;
		DrawBar(Right - BarW, Y + 76.f * UIScale, BarW, 6.f * UIScale, Fraction,
			Fraction < 0.25f ? FLinearColor(0.9f, 0.3f, 0.2f) : FLinearColor(0.7f, 0.7f, 0.7f));
	}
}

void AZSHUD::DrawClockAndCash(const AZSCharacter& Player)
{
	const float W = Canvas->SizeX;

	DrawLabel(FString::Printf(TEXT("$ %d"), Player.GetInventory()->GetMoney()), W - 40.f * UIScale, 30.f * UIScale, kGold, 1.6f, false, true);

	if (const UZSWorldDirector* Director = UZSWorldDirector::Get(&Player))
	{
		const bool bNight = Director->IsNight();
		const FString Clock = FString::Printf(TEXT("Day %d   %s%s"), Director->GetDayNumber(), *Director->GetTimeText().ToString(), bNight ? TEXT("   NIGHT") : TEXT(""));
		DrawLabel(Clock, 40.f * UIScale, 30.f * UIScale, bNight ? FLinearColor(1.f, 0.35f, 0.3f) : kText, 1.2f);
	}

	if (const AZSSafeZone* Zone = Player.GetCurrentSafeZone())
	{
		DrawLabel(FString::Printf(TEXT("SAFE ZONE - %s"), *Zone->ZoneName.ToString()), W * 0.5f, 30.f * UIScale, FLinearColor(0.4f, 1.f, 0.5f), 1.2f, true);
	}
}

void AZSHUD::DrawCrosshairAndPrompt(const AZSCharacter& Player, const AZSPlayerController& Controller)
{
	if (Player.IsDead() || Controller.IsTrading())
	{
		return;
	}

	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	const float Arm = 7.f * UIScale;
	DrawLine(CX - Arm, CY, CX + Arm, CY, FLinearColor(1.f, 1.f, 1.f, 0.85f), 1.5f);
	DrawLine(CX, CY - Arm, CX, CY + Arm, FLinearColor(1.f, 1.f, 1.f, 0.85f), 1.5f);

	const FText Prompt = Player.GetFocusPrompt();
	if (!Prompt.IsEmpty())
	{
		DrawLabel(FString::Printf(TEXT("[E]  %s"), *Prompt.ToString()), CX, CY + 60.f * UIScale, kText, 1.2f, true);
	}
}

void AZSHUD::DrawNotifications(const AZSPlayerController& Controller)
{
	const float Right = Canvas->SizeX - 40.f * UIScale;
	float Y = Canvas->SizeY * 0.35f;
	for (const FZSNotification& Note : Controller.GetNotifications())
	{
		Y += DrawLabel(Note.Text, Right, Y, Note.Color, 1.f, false, true) + 6.f * UIScale;
	}
}

void AZSHUD::DrawInventory(const AZSCharacter& Player)
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(&Player);
	const UZSInventoryComponent* Inventory = Player.GetInventory();

	const float PanelW = 560.f * UIScale;
	const float PanelH = Canvas->SizeY * 0.75f;
	const float X = Canvas->SizeX * 0.5f - PanelW * 0.5f;
	const float Y = Canvas->SizeY * 0.5f - PanelH * 0.5f;
	DrawRect(kPanel, X, Y, PanelW, PanelH);

	DrawLabel(FString::Printf(TEXT("INVENTORY  (%d / %d)"), Inventory->GetSlots().Num(), Inventory->MaxSlots), X + 20.f * UIScale, Y + 14.f * UIScale, kGold, 1.2f);
	DrawLabel(FString::Printf(TEXT("$ %d"), Inventory->GetMoney()), X + PanelW - 20.f * UIScale, Y + 14.f * UIScale, kGold, 1.2f, false, true);

	float RowY = Y + 56.f * UIScale;
	const float RowH = 24.f * UIScale;
	const int32 Equipped = Player.GetCombat()->GetEquippedSlot();
	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	for (int32 Index = 0; Index < Slots.Num() && RowY < Y + PanelH - RowH; ++Index)
	{
		const FString Prefix = Index == Equipped ? TEXT("> ") : TEXT("  ");
		DrawLabel(Prefix + DescribeStack(Slots[Index], Items), X + 20.f * UIScale, RowY, Index == Equipped ? kGold : kText, 0.95f);
		RowY += RowH;
	}

	DrawLabel(TEXT("I: close   Wheel: switch weapon   H: heal   R: reload"), X + PanelW * 0.5f, Y + PanelH - 30.f * UIScale, kDim, 0.8f, true);
}

void AZSHUD::DrawTrade(const AZSCharacter& Player, const AZSPlayerController& Controller)
{
	const UZSMerchantComponent* Merchant = Controller.GetTradeMerchant();
	if (!Merchant || !Merchant->GetStock())
	{
		return;
	}

	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(&Player);
	const UZSInventoryComponent* Stock = Merchant->GetStock();
	const UZSInventoryComponent* Mine = Player.GetInventory();

	const float PanelW = Canvas->SizeX * 0.72f;
	const float PanelH = Canvas->SizeY * 0.78f;
	const float X = Canvas->SizeX * 0.5f - PanelW * 0.5f;
	const float Y = Canvas->SizeY * 0.5f - PanelH * 0.5f;
	const float ColW = PanelW * 0.5f;
	DrawRect(kPanel, X, Y, PanelW, PanelH);

	DrawLabel(Merchant->MerchantName.ToString(), X + 24.f * UIScale, Y + 14.f * UIScale, kGold, 1.5f);
	DrawLabel(FString::Printf(TEXT("Their cash: $%d      Your cash: $%d"), Stock->GetMoney(), Mine->GetMoney()), X + PanelW - 24.f * UIScale, Y + 20.f * UIScale, kText, 1.f, false, true);

	const float RowH = 26.f * UIScale;
	const float ListTop = Y + 84.f * UIScale;
	const int32 MaxRows = FMath::Max(1, FMath::FloorToInt((PanelH - 84.f * UIScale - 90.f * UIScale) / RowH));

	auto DrawColumn = [&](bool bSell, float ColX)
	{
		const bool bActive = Controller.IsSellColumn() == bSell;
		const UZSInventoryComponent* Source = bSell ? Mine : Stock;
		const TArray<FZSItemStack>& Slots = Source->GetSlots();

		DrawLabel(bSell ? TEXT("SELL  (your items)") : TEXT("BUY  (their stock)"), ColX + 24.f * UIScale, Y + 54.f * UIScale, bActive ? kGold : kDim, 1.1f);

		// Scroll so the cursor stays visible.
		const int32 Cursor = bActive ? Controller.GetTradeCursor() : 0;
		const int32 First = FMath::Clamp(Cursor - MaxRows + 1, 0, FMath::Max(0, Slots.Num() - MaxRows));

		if (Slots.Num() == 0)
		{
			DrawLabel(TEXT("(nothing)"), ColX + 24.f * UIScale, ListTop, kDim, 0.95f);
		}

		for (int32 Row = 0; Row < MaxRows && First + Row < Slots.Num(); ++Row)
		{
			const int32 Index = First + Row;
			const FZSItemStack& Stack = Slots[Index];
			const float RowY = ListTop + Row * RowH;

			if (bActive && Index == Controller.GetTradeCursor())
			{
				DrawRect(kSelect, ColX + 12.f * UIScale, RowY - 2.f * UIScale, ColW - 24.f * UIScale, RowH);
			}

			// Price of one unit: what you pay when buying, what you are offered when selling.
			FString Price;
			if (bSell)
			{
				const int32 Offer = Merchant->GetSellPrice(Stack, 1);
				Price = Offer > 0 ? FString::Printf(TEXT("$%d"), Offer) : FString(TEXT("--"));
			}
			else
			{
				Price = FString::Printf(TEXT("$%d"), Merchant->GetBuyPrice(Stack));
			}

			DrawLabel(DescribeStack(Stack, Items), ColX + 24.f * UIScale, RowY, bSell && Merchant->GetSellPrice(Stack, 1) == 0 ? kDim : kText, 0.9f);
			DrawLabel(Price, ColX + ColW - 28.f * UIScale, RowY, kGold, 0.9f, false, true);
		}
	};

	DrawColumn(false, X);
	DrawColumn(true, X + ColW);

	DrawLabel(Controller.GetTradeMessage(), X + PanelW * 0.5f, Y + PanelH - 60.f * UIScale, Controller.GetTradeMessageColor(), 1.f, true);
	DrawLabel(TEXT("Up/Down: select   Enter: trade one   Shift+Enter: five   Tab: switch column   E / Backspace: leave"), X + PanelW * 0.5f, Y + PanelH - 30.f * UIScale, kDim, 0.8f, true);
}
