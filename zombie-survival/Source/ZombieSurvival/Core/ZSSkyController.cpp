#include "Core/ZSSkyController.h"
#include "Core/ZSWorldDirector.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"

AZSSkyController::AZSSkyController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.05f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AZSSkyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UZSWorldDirector* Director = UZSWorldDirector::Get(this);
	if (!Director || !Sun)
	{
		return;
	}

	// 6h = sunrise (pitch 0), 12h = zenith (-90), 18h = sunset (-180).
	const float Hours = Director->GetTimeOfDay();
	float Pitch = -(Hours - 6.f) * 15.f;
	const float Elevation = FMath::Sin(FMath::DegreesToRadians(-Pitch)); // +1 noon .. -1 midnight

	// Below the horizon the same light becomes the moon, shining from above.
	if (Elevation < 0.f)
	{
		Pitch = -FMath::Lerp(15.f, 55.f, FMath::Clamp(-Elevation, 0.f, 1.f));
	}
	Sun->SetActorRotation(FRotator(Pitch, SunYaw, 0.f));

	const float DayAlpha = FMath::SmoothStep(-0.05f, 0.25f, Elevation);
	const float Intensity = FMath::Lerp(NightMoonIntensity, DaySunIntensity, DayAlpha);

	// Warm near the horizon, neutral at noon, blue at night.
	const float HorizonAlpha = 1.f - FMath::Clamp(FMath::Abs(Elevation) * 3.f, 0.f, 1.f);
	FLinearColor Color = FLinearColor::LerpUsingHSV(NightColor, DayColor, DayAlpha);
	Color = FLinearColor::LerpUsingHSV(Color, DuskColor, HorizonAlpha * 0.6f * (Elevation > -0.1f ? 1.f : 0.f));

	if (ULightComponent* Light = Sun->GetLightComponent())
	{
		Light->SetIntensity(Intensity);
		Light->SetLightColor(Color);
	}

	RecaptureTimer += DeltaSeconds;
	if (Sky && RecaptureTimer >= RecaptureInterval)
	{
		RecaptureTimer = 0.f;
		if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
		{
			SkyComponent->RecaptureSky();
		}
	}
}
