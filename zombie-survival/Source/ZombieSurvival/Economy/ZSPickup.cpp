#include "Economy/ZSPickup.h"
#include "Core/ZSPlaceholder.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"
#include "Player/ZSCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AZSPickup::AZSPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(70.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // so the interaction sweep finds it
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	ZSPlaceholder::SetShape(Visual, EZSBasicShape::Cube);
}

void AZSPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Visual->AddRelativeRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
}

void AZSPickup::Configure(const FZSItemStack& InItem, int32 InCash)
{
	Item = InItem;
	Cash = FMath::Max(0, InCash);

	if (Cash > 0)
	{
		Visual->SetRelativeScale3D(FVector(0.28f, 0.16f, 0.04f));
		ZSPlaceholder::SetColor(Visual, FLinearColor(0.15f, 0.7f, 0.2f));
	}
	else
	{
		Visual->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.3f));
		ZSPlaceholder::SetColor(Visual, FLinearColor(0.9f, 0.7f, 0.15f));
	}
}

AZSPickup* AZSPickup::SpawnItem(UWorld* World, const FVector& Location, const FZSItemStack& Item, float LifeSeconds)
{
	if (!World || Item.IsEmpty())
	{
		return nullptr;
	}

	// Drop to the ground so nothing hovers.
	FVector Spot = Location;
	FHitResult Ground;
	if (World->LineTraceSingleByChannel(Ground, Location + FVector(0.f, 0.f, 100.f), Location - FVector(0.f, 0.f, 500.f), ECC_WorldStatic))
	{
		Spot = Ground.ImpactPoint + FVector(0.f, 0.f, 12.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AZSPickup* Pickup = World->SpawnActor<AZSPickup>(AZSPickup::StaticClass(), Spot, FRotator::ZeroRotator, Params);
	if (Pickup)
	{
		Pickup->Configure(Item, 0);
		Pickup->SetLifeSpan(LifeSeconds);
	}
	return Pickup;
}

AZSPickup* AZSPickup::SpawnCash(UWorld* World, const FVector& Location, int32 Amount, float LifeSeconds)
{
	if (!World || Amount <= 0)
	{
		return nullptr;
	}

	FVector Spot = Location;
	FHitResult Ground;
	if (World->LineTraceSingleByChannel(Ground, Location + FVector(0.f, 0.f, 100.f), Location - FVector(0.f, 0.f, 500.f), ECC_WorldStatic))
	{
		Spot = Ground.ImpactPoint + FVector(0.f, 0.f, 12.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AZSPickup* Pickup = World->SpawnActor<AZSPickup>(AZSPickup::StaticClass(), Spot, FRotator::ZeroRotator, Params);
	if (Pickup)
	{
		Pickup->Configure(FZSItemStack(), Amount);
		Pickup->SetLifeSpan(LifeSeconds);
	}
	return Pickup;
}

void AZSPickup::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AZSPickup::OnTriggerBeginOverlap);

	// Cash dropped on top of the player (e.g. where they died) would otherwise never fire a begin-overlap.
	if (Cash > 0)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, AZSCharacter::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			OnTriggerBeginOverlap(Trigger, Actor, nullptr, 0, false, FHitResult());
		}
	}
}

void AZSPickup::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cash <= 0 || bCollected)
	{
		return;
	}

	AZSCharacter* Character = Cast<AZSCharacter>(OtherActor);
	if (!Character || Character->IsDead())
	{
		return;
	}

	bCollected = true;
	Character->GetInventory()->AddMoney(Cash);
	Character->Notify(FText::FromString(FString::Printf(TEXT("+$%d"), Cash)), FLinearColor(0.4f, 1.f, 0.4f));
	Destroy();
}

bool AZSPickup::CanInteract(const AActor* Interactor) const
{
	return !bCollected && Cash <= 0 && !Item.IsEmpty();
}

FText AZSPickup::GetInteractPrompt(const AActor* Interactor) const
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	const FText Name = Items ? Items->GetDisplayName(Item.ItemId) : FText::FromName(Item.ItemId);
	return FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *Name.ToString(), Item.Quantity));
}

void AZSPickup::Interact(AActor* Interactor)
{
	AZSCharacter* Character = Cast<AZSCharacter>(Interactor);
	if (!Character || bCollected || Item.IsEmpty())
	{
		return;
	}

	const int32 Leftover = Character->GetInventory()->AddStack(Item);
	if (Leftover <= 0)
	{
		bCollected = true;
		Destroy();
	}
	else
	{
		Item.Quantity = Leftover;
		Character->Notify(FText::FromString(TEXT("Inventory full")), FLinearColor(1.f, 0.6f, 0.2f));
	}
}
