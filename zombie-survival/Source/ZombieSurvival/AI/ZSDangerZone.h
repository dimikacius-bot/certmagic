#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSDangerZone.generated.h"

class USphereComponent;

/**
 * Marks an infested area (hospital, military checkpoint, downtown...). While the player is inside,
 * the zombie budget is multiplied. Overlapping zones do not stack: the highest multiplier wins.
 * Use values below 1 for quiet countryside.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSDangerZone : public AActor
{
	GENERATED_BODY()

public:
	AZSDangerZone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger Zone", meta = (ClampMin = "0.0"))
	float DensityMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger Zone")
	FText ZoneName = FText::FromString(TEXT("Infested Area"));

	bool ContainsPoint(const FVector& WorldPoint) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Area;
};
