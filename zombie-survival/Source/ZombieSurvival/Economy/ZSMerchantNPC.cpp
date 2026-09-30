#include "Economy/ZSMerchantNPC.h"
#include "Core/ZSPlaceholder.h"
#include "Economy/ZSMerchantComponent.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Player/ZSPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AZSMerchantNPC::AZSMerchantNPC()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	AutoPossessAI = EAutoPossessAI::Disabled;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // interaction sweep
	GetCharacterMovement()->DisableMovement();

	Stock = CreateDefaultSubobject<UZSInventoryComponent>(TEXT("Stock"));
	Merchant = CreateDefaultSubobject<UZSMerchantComponent>(TEXT("Merchant"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f));
	ZSPlaceholder::SetShape(PlaceholderBody, EZSBasicShape::Cylinder);

	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(GetCapsuleComponent());
	NameLabel->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetWorldSize(26.f);
	NameLabel->SetTextRenderColor(FColor(120, 200, 255));
}

void AZSMerchantNPC::PostInitializeComponents()
{
	ApplyArchetype();
	Super::PostInitializeComponents();

	ZSPlaceholder::SetColor(PlaceholderBody, FLinearColor(0.15f, 0.35f, 0.75f));
	NameLabel->SetText(DisplayName);

	// Skeletal mesh assigned in a Blueprint subclass replaces the stand-in.
	if (GetMesh() && GetMesh()->GetSkeletalMeshAsset())
	{
		PlaceholderBody->SetVisibility(false);
	}
}

void AZSMerchantNPC::ApplyArchetype()
{
	using Cat = EZSItemCategory;

	// Stock, categories and name are only filled in when left empty. Cash and price rates always come from
	// the archetype: pick Custom to keep whatever is set on the Merchant component.
	const bool bHasStock = Merchant->DefaultStock.Num() > 0;
	const bool bHasCategories = Merchant->BuysCategories.Num() > 0;

	auto SetName = [this](const TCHAR* Name)
	{
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(Name);
		}
		Merchant->MerchantName = DisplayName;
	};

	switch (MerchantType)
	{
	case EZSMerchantType::General:
		SetName(TEXT("General Store"));
		if (!bHasCategories)
		{
			Merchant->BuysCategories = { Cat::Junk, Cat::Material, Cat::Food, Cat::Medical, Cat::Melee };
		}
		if (!bHasStock)
		{
			Merchant->DefaultStock = {
				{ TEXT("Bandage"), 6 }, { TEXT("CannedFood"), 8 }, { TEXT("EnergyBar"), 8 }, { TEXT("Scrap"), 30 },
				{ TEXT("Cloth"), 20 }, { TEXT("DuctTape"), 4 }, { TEXT("Pipe"), 2 }, { TEXT("BaseballBat"), 1 } };
		}
		Merchant->StartingCash = Merchant->MaxCash = 700;
		break;

	case EZSMerchantType::Gunsmith:
		SetName(TEXT("Armory"));
		if (!bHasCategories)
		{
			Merchant->BuysCategories = { Cat::Firearm, Cat::Ammo, Cat::Melee };
		}
		if (!bHasStock)
		{
			Merchant->DefaultStock = {
				{ TEXT("Pistol9mm"), 2 }, { TEXT("Shotgun"), 1 }, { TEXT("HuntingRifle"), 1 }, { TEXT("Ammo9mm"), 60 },
				{ TEXT("AmmoShell"), 24 }, { TEXT("AmmoRifle"), 20 }, { TEXT("Machete"), 1 }, { TEXT("FireAxe"), 1 } };
		}
		Merchant->SellMarkup = 1.45f;
		Merchant->StartingCash = 1500;
		Merchant->MaxCash = 3000;
		break;

	case EZSMerchantType::Medic:
		SetName(TEXT("Field Clinic"));
		if (!bHasCategories)
		{
			Merchant->BuysCategories = { Cat::Medical, Cat::Food };
		}
		if (!bHasStock)
		{
			Merchant->DefaultStock = { { TEXT("Bandage"), 10 }, { TEXT("Medkit"), 4 }, { TEXT("Antibiotics"), 3 }, { TEXT("CannedFood"), 4 } };
		}
		Merchant->StartingCash = Merchant->MaxCash = 900;
		break;

	case EZSMerchantType::Fence:
		SetName(TEXT("The Fence"));
		// Buys everything (empty category list) and pays the best rate, but sells dear.
		if (!bHasStock)
		{
			Merchant->DefaultStock = { { TEXT("Watch"), 3 }, { TEXT("Jewelry"), 2 }, { TEXT("Electronics"), 4 }, { TEXT("Radio"), 2 } };
		}
		Merchant->BuyRate = 0.5f;
		Merchant->SellMarkup = 1.6f;
		Merchant->StartingCash = 2000;
		Merchant->MaxCash = 4000;
		break;

	case EZSMerchantType::Custom:
		SetName(TEXT("Trader"));
		break;
	}
}

void AZSMerchantNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the name tag facing the camera.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FVector ToCamera = Camera->GetCameraLocation() - NameLabel->GetComponentLocation();
		ToCamera.Z = 0.f;
		if (!ToCamera.IsNearlyZero())
		{
			NameLabel->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

FText AZSMerchantNPC::GetInteractPrompt(const AActor* Interactor) const
{
	return FText::FromString(FString::Printf(TEXT("Trade with %s"), *DisplayName.ToString()));
}

void AZSMerchantNPC::Interact(AActor* Interactor)
{
	if (const APawn* Pawn = Cast<APawn>(Interactor))
	{
		if (AZSPlayerController* Controller = Cast<AZSPlayerController>(Pawn->GetController()))
		{
			Controller->OpenTrade(this);
		}
	}
}
