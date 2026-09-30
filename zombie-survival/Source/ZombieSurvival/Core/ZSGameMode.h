#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZSGameMode.generated.h"

class AZSSafeZone;

/**
 * Wires the default classes together and remembers the last safe zone the player reached, which is
 * where they wake up after dying. Subclass in Blueprint (BP_ZSGameMode) to plug in your own pawn,
 * controller or HUD classes once the art exists.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZSGameMode();

	void SetLastSafeZone(AZSSafeZone* Zone) { LastSafeZone = Zone; }

	/** Last safe zone's respawn point, or a player start when none has been reached yet. */
	FTransform GetRespawnTransform(AController* Controller) const;

private:
	TWeakObjectPtr<AZSSafeZone> LastSafeZone;
};
