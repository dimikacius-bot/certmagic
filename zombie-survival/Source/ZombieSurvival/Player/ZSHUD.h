#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZSHUD.generated.h"

class AZSCharacter;
class AZSPlayerController;
class UZSItemSubsystem;
struct FZSItemStack;

/**
 * Canvas HUD: vitals, cash, clock, weapon, prompt, message feed, inventory list and the trade window.
 * Deliberately asset-free so the game is playable without UMG; replace it with widgets when the art
 * pipeline is in place (all data it shows is available through public getters).
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill);
	/** Text helper (AHUD::DrawLine already draws lines). Returns the line height used. */
	float DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false, bool bRightAlign = false);
	FString DescribeStack(const FZSItemStack& Stack, const UZSItemSubsystem* Items) const;

	void DrawVitals(const AZSCharacter& Player);
	void DrawWeapon(const AZSCharacter& Player);
	void DrawClockAndCash(const AZSCharacter& Player);
	void DrawCrosshairAndPrompt(const AZSCharacter& Player, const AZSPlayerController& Controller);
	void DrawNotifications(const AZSPlayerController& Controller);
	void DrawInventory(const AZSCharacter& Player);
	void DrawTrade(const AZSCharacter& Player, const AZSPlayerController& Controller);
	void DrawOverlays(const AZSCharacter& Player);

	float UIScale = 1.f;
};
