#include "Core/ZSGameMode.h"
#include "Economy/ZSSafeZone.h"
#include "Player/ZSCharacter.h"
#include "Player/ZSHUD.h"
#include "Player/ZSPlayerController.h"

AZSGameMode::AZSGameMode()
{
	DefaultPawnClass = AZSCharacter::StaticClass();
	PlayerControllerClass = AZSPlayerController::StaticClass();
	HUDClass = AZSHUD::StaticClass();
}

FTransform AZSGameMode::GetRespawnTransform(AController* Controller) const
{
	if (const AZSSafeZone* Zone = LastSafeZone.Get())
	{
		return Zone->GetRespawnTransform();
	}

	if (AActor* Start = const_cast<AZSGameMode*>(this)->FindPlayerStart(Controller))
	{
		return Start->GetActorTransform();
	}
	return FTransform(FVector(0.f, 0.f, 300.f));
}
