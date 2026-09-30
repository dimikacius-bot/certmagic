#include "Economy/ZSLootContainer.h"
#include "Core/ZSPlaceholder.h"
#include "Core/ZSWorldDirector.h"
#include "Economy/ZSPickup.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"
#include "Player/ZSCharacter.h"
#include "Components/StaticMeshComponent.h"

AZSLootContainer::AZSLootContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetRelativeScale3D(FVector(0.9f, 0.5f, 0.5f));
	RootComponent = Mesh;
	ZSPlaceholder::SetShape(Mesh, EZSBasicShape::Cube);
}

void AZSLootContainer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisual();
}

void AZSLootContainer::BeginPlay()
{
	Super::BeginPlay();

	if (UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		Director->OnNewDay.AddDynamic(this, &AZSLootContainer::HandleNewDay);
	}
}

void AZSLootContainer::UpdateVisual()
{
	// Warm brown while full, dark grey once emptied.
	ZSPlaceholder::SetColor(Mesh, bLooted ? FLinearColor(0.08f, 0.08f, 0.08f) : FLinearColor(0.35f, 0.22f, 0.1f));
}

void AZSLootContainer::HandleNewDay(int32 DayNumber)
{
	if (bLooted && RefillAfterDays > 0 && DayNumber - LootedOnDay >= RefillAfterDays)
	{
		bLooted = false;
		UpdateVisual();
	}
}

bool AZSLootContainer::CanInteract(const AActor* Interactor) const
{
	return !bLooted;
}

FText AZSLootContainer::GetInteractPrompt(const AActor* Interactor) const
{
	return FText::FromString(FString::Printf(TEXT("Search %s"), *ContainerName.ToString()));
}

void AZSLootContainer::Interact(AActor* Interactor)
{
	AZSCharacter* Character = Cast<AZSCharacter>(Interactor);
	if (!Character || bLooted)
	{
		return;
	}

	bLooted = true;
	if (const UZSWorldDirector* Director = UZSWorldDirector::Get(this))
	{
		LootedOnDay = Director->GetDayNumber();
	}
	UpdateVisual();

	const FZSLootResult Loot = ZSLoot::Roll(this, LootTable);
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);

	if (Loot.Items.Num() == 0 && Loot.Cash == 0)
	{
		Character->Notify(FText::FromString(TEXT("Nothing useful.")), FLinearColor(0.7f, 0.7f, 0.7f));
		return;
	}

	for (const FZSItemStack& Stack : Loot.Items)
	{
		const int32 Leftover = Character->GetInventory()->AddStack(Stack);
		const int32 Added = Stack.Quantity - Leftover;
		if (Added > 0)
		{
			const FText Name = Items ? Items->GetDisplayName(Stack.ItemId) : FText::FromName(Stack.ItemId);
			Character->Notify(FText::FromString(FString::Printf(TEXT("+%d %s"), Added, *Name.ToString())), FLinearColor(0.9f, 0.9f, 0.6f));
		}
		if (Leftover > 0)
		{
			FZSItemStack Spill = Stack;
			Spill.Quantity = Leftover;
			AZSPickup::SpawnItem(GetWorld(), Character->GetActorLocation(), Spill);
			Character->Notify(FText::FromString(TEXT("Inventory full - dropped the rest")), FLinearColor(1.f, 0.6f, 0.2f));
		}
	}

	if (Loot.Cash > 0)
	{
		Character->GetInventory()->AddMoney(Loot.Cash);
		Character->Notify(FText::FromString(FString::Printf(TEXT("+$%d"), Loot.Cash)), FLinearColor(0.4f, 1.f, 0.4f));
	}
}
