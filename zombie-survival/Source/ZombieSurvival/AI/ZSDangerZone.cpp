#include "AI/ZSDangerZone.h"
#include "Core/ZSWorldDirector.h"
#include "Components/SphereComponent.h"

AZSDangerZone::AZSDangerZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Area = CreateDefaultSubobject<USphereComponent>(TEXT("Area"));
	Area->InitSphereRadius(8000.f);
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->ShapeColor = FColor(220, 60, 40);
	RootComponent = Area;
}

void AZSDangerZone::BeginPlay()
{
	Super::BeginPlay();
	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->RegisterDangerZone(this);
	}
}

void AZSDangerZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->UnregisterDangerZone(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AZSDangerZone::ContainsPoint(const FVector& WorldPoint) const
{
	return FVector::DistSquared(WorldPoint, Area->GetComponentLocation()) <= FMath::Square(Area->GetScaledSphereRadius());
}
