#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSSkyController.generated.h"

class ADirectionalLight;
class ASkyLight;

/**
 * Drives the sun / moon and the sky capture from the world clock. Place one in the level and assign
 * the level's Directional Light (Mobility = Movable, "Atmosphere Sun Light" on) and Sky Light
 * (Real Time Capture is even better; otherwise this recaptures periodically).
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSSkyController : public AActor
{
	GENERATED_BODY()

public:
	AZSSkyController();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky")
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky")
	TObjectPtr<ASkyLight> Sky;

	/** Compass heading of the sun's path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	float SunYaw = 40.f;

	/** Lux at noon (the engine default for a Directional Light is 10 - real sunlight is ~100000). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	float DaySunIntensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	float NightMoonIntensity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	FLinearColor DayColor = FLinearColor(1.f, 0.95f, 0.88f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	FLinearColor DuskColor = FLinearColor(1.f, 0.5f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	FLinearColor NightColor = FLinearColor(0.45f, 0.55f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky", meta = (Units = "s"))
	float RecaptureInterval = 8.f;

private:
	float RecaptureTimer = 0.f;
};
