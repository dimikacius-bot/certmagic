#include "Combat/ZSCombatComponent.h"
#include "Inventory/ZSInventoryComponent.h"
#include "Inventory/ZSItemSubsystem.h"
#include "Player/ZSCharacter.h"
#include "AI/ZSZombie.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

UZSCombatComponent::UZSCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZSCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UZSInventoryComponent>() : nullptr;
	if (Inventory)
	{
		Inventory->OnChanged.AddDynamic(this, &UZSCombatComponent::HandleInventoryChanged);
	}
}

// --- Equipment --------------------------------------------------------------------------------

FZSItemStack* UZSCombatComponent::GetEquippedMutable()
{
	if (!Inventory || EquippedSlot == INDEX_NONE)
	{
		return nullptr;
	}
	TArray<FZSItemStack>& Slots = Inventory->GetMutableSlots();
	return Slots.IsValidIndex(EquippedSlot) ? &Slots[EquippedSlot] : nullptr;
}

const FZSItemStack* UZSCombatComponent::GetEquippedStack() const
{
	if (!Inventory || EquippedSlot == INDEX_NONE)
	{
		return nullptr;
	}
	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	return Slots.IsValidIndex(EquippedSlot) ? &Slots[EquippedSlot] : nullptr;
}

const FZSItemRow* UZSCombatComponent::GetEquippedRow() const
{
	const FZSItemStack* Stack = GetEquippedStack();
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	return (Stack && Items) ? Items->Find(Stack->ItemId) : nullptr;
}

bool UZSCombatComponent::Equip(int32 SlotIndex)
{
	if (!Inventory)
	{
		return false;
	}

	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	if (!Items || !Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FZSItemRow* Row = Items->Find(Slots[SlotIndex].ItemId);
	if (!Row || !Row->IsWeapon())
	{
		return false;
	}

	CancelReload();
	EquippedSlot = SlotIndex;
	EquippedId = Slots[SlotIndex].ItemId;
	NextAttackTime = FMath::Max(NextAttackTime, GetWorld()->GetTimeSeconds() + 0.25f);
	OnEquipChanged.Broadcast();
	return true;
}

void UZSCombatComponent::Unequip()
{
	if (EquippedSlot == INDEX_NONE && EquippedId.IsNone())
	{
		return;
	}
	CancelReload();
	EquippedSlot = INDEX_NONE;
	EquippedId = NAME_None;
	OnEquipChanged.Broadcast();
}

void UZSCombatComponent::CycleWeapon(int32 Direction)
{
	if (!Inventory)
	{
		return;
	}

	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	if (!Items)
	{
		return;
	}

	// Options: INDEX_NONE (unarmed) followed by every weapon slot.
	TArray<int32> Options;
	Options.Add(INDEX_NONE);
	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FZSItemRow* Row = Items->Find(Slots[Index].ItemId);
		if (Row && Row->IsWeapon())
		{
			Options.Add(Index);
		}
	}

	int32 Current = Options.IndexOfByKey(EquippedSlot);
	if (Current == INDEX_NONE)
	{
		Current = 0;
	}

	const int32 Next = (Current + (Direction >= 0 ? 1 : -1) + Options.Num()) % Options.Num();
	if (Options[Next] == INDEX_NONE)
	{
		Unequip();
	}
	else
	{
		Equip(Options[Next]);
	}
}

void UZSCombatComponent::AutoEquipBest()
{
	if (!Inventory)
	{
		return;
	}

	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	if (!Items)
	{
		return;
	}

	int32 BestSlot = INDEX_NONE;
	float BestDamage = -1.f;
	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FZSItemRow* Row = Items->Find(Slots[Index].ItemId);
		if (Row && Row->Category == EZSItemCategory::Melee && Row->Damage > BestDamage)
		{
			BestDamage = Row->Damage;
			BestSlot = Index;
		}
	}

	if (BestSlot != INDEX_NONE)
	{
		Equip(BestSlot);
	}
}

void UZSCombatComponent::HandleInventoryChanged()
{
	if (EquippedId.IsNone() || !Inventory)
	{
		return;
	}

	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	if (Slots.IsValidIndex(EquippedSlot) && Slots[EquippedSlot].ItemId == EquippedId)
	{
		return;
	}

	// Slots shifted (or the weapon was sold / broke): find another copy, otherwise holster.
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].ItemId == EquippedId)
		{
			EquippedSlot = Index;
			OnEquipChanged.Broadcast();
			return;
		}
	}
	Unequip();
}

// --- Attacks ----------------------------------------------------------------------------------

void UZSCombatComponent::GetAim(FVector& OutLocation, FVector& OutDirection) const
{
	FRotator Rotation = FRotator::ZeroRotator;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(OutLocation, Rotation);
			OutDirection = Rotation.Vector();
			return;
		}
	}
	GetOwner()->GetActorEyesViewPoint(OutLocation, Rotation);
	OutDirection = Rotation.Vector();
}

bool UZSCombatComponent::PrimaryAttack()
{
	AZSCharacter* Owner = Cast<AZSCharacter>(GetOwner());
	if (!Owner || Owner->IsDead() || bReloading || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return false;
	}

	const FZSItemRow* Row = GetEquippedRow();
	if (Row && Row->IsFirearm())
	{
		return DoFire(*Owner, *Row);
	}
	return DoMelee(*Owner, (Row && Row->IsWeapon()) ? Row : nullptr);
}

bool UZSCombatComponent::DoMelee(AZSCharacter& Owner, const FZSItemRow* Row)
{
	const float Damage = Row ? Row->Damage : UnarmedDamage;
	const float Range = Row ? Row->Range : UnarmedRange;
	const float Interval = Row ? Row->AttackInterval : UnarmedInterval;
	const float StaminaCost = Row ? Row->StaminaCost : UnarmedStamina;
	const float Noise = Row ? Row->Noise : 300.f;

	if (!Owner.TrySpendStamina(StaminaCost))
	{
		return false;
	}

	UWorld* World = GetWorld();
	NextAttackTime = World->GetTimeSeconds() + Interval;

	FVector Start, Dir;
	GetAim(Start, Dir);

	// Cleave: everything inside a sphere in front of the camera, with line of sight.
	const float Radius = Range * 0.55f;
	const FVector Center = Start + Dir * Radius;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZSMelee), false, &Owner);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);

	int32 Hits = 0;
	TSet<AZSZombie*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AZSZombie* Zombie = Cast<AZSZombie>(Overlap.GetActor());
		if (!Zombie || Zombie->IsDead() || Seen.Contains(Zombie))
		{
			continue;
		}
		Seen.Add(Zombie);

		const FVector ToZombie = Zombie->GetActorLocation() - Start;
		if (FVector::DotProduct(ToZombie.GetSafeNormal(), Dir) < 0.25f)
		{
			continue;
		}

		FCollisionQueryParams LosParams(SCENE_QUERY_STAT(ZSMeleeLos), false, &Owner);
		LosParams.AddIgnoredActor(Zombie);
		if (World->LineTraceTestByChannel(Start, Zombie->GetActorLocation(), ECC_Visibility, LosParams))
		{
			continue;
		}

		UGameplayStatics::ApplyPointDamage(Zombie, Damage, Dir, FHitResult(), Owner.GetController(), &Owner, UDamageType::StaticClass());
		Zombie->ApplyStagger(0.55f);
		Zombie->LaunchCharacter(Dir.GetSafeNormal2D() * 320.f + FVector(0.f, 0.f, 90.f), true, false);
		++Hits;
	}

	ReportNoise(Noise, TEXT("Melee"));
	OnAttack.Broadcast(false);

	if (Row && Hits > 0)
	{
		ConsumeDurability(1);
	}
	return true;
}

bool UZSCombatComponent::DoFire(AZSCharacter& Owner, const FZSItemRow& Row)
{
	FZSItemStack* Stack = GetEquippedMutable();
	if (!Stack)
	{
		return false;
	}

	if (Stack->AmmoInMag <= 0)
	{
		StartReload();
		return false;
	}

	UWorld* World = GetWorld();
	NextAttackTime = World->GetTimeSeconds() + Row.AttackInterval;
	--Stack->AmmoInMag;

	FVector Start, Dir;
	GetAim(Start, Dir);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZSFire), true, &Owner);

	for (int32 Pellet = 0; Pellet < FMath::Max(1, Row.Pellets); ++Pellet)
	{
		const FVector ShotDir = Row.SpreadDegrees > 0.f ? FMath::VRandCone(Dir, FMath::DegreesToRadians(Row.SpreadDegrees)) : Dir;
		const FVector End = Start + ShotDir * Row.Range;

		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

		if (bHit)
		{
			if (AZSZombie* Zombie = Cast<AZSZombie>(Hit.GetActor()))
			{
				const float CapsuleTop = Zombie->GetActorLocation().Z + Zombie->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const bool bHeadshot = Hit.ImpactPoint.Z > CapsuleTop - 28.f;
				const float Damage = Row.Damage * (bHeadshot ? HeadshotMultiplier : 1.f);

				UGameplayStatics::ApplyPointDamage(Zombie, Damage, ShotDir, Hit, Owner.GetController(), &Owner, UDamageType::StaticClass());
				Zombie->ApplyStagger(0.2f);
			}
		}

#if !UE_BUILD_SHIPPING
		// Placeholder tracer until real VFX exist.
		const FVector Muzzle = Start + Dir * 60.f - FVector(0.f, 0.f, 15.f);
		DrawDebugLine(World, Muzzle, bHit ? Hit.ImpactPoint : End, FColor::Yellow, false, 0.06f, 0, 1.5f);
#endif
	}

	ReportNoise(Row.Noise, TEXT("Gunshot"));
	Owner.AddControllerPitchInput(-0.35f);
	OnAttack.Broadcast(true);

	// May remove the slot (weapon breaks): keep this last, `Stack` is not valid afterwards.
	ConsumeDurability(1);
	return true;
}

void UZSCombatComponent::ConsumeDurability(int32 Amount)
{
	FZSItemStack* Stack = GetEquippedMutable();
	if (!Stack)
	{
		return;
	}

	Stack->Durability -= Amount;
	if (Stack->Durability <= 0)
	{
		const FName BrokenId = Stack->ItemId;
		FZSItemStack Removed;
		Inventory->RemoveFromSlot(EquippedSlot, 1, Removed); // OnChanged -> HandleInventoryChanged holsters
		OnWeaponBroke.Broadcast(BrokenId);
	}
}

void UZSCombatComponent::ReportNoise(float Range, FName Tag)
{
	if (Range > 0.f && GetOwner())
	{
		UAISense_Hearing::ReportNoiseEvent(this, GetOwner()->GetActorLocation(), 1.f, GetOwner(), Range, Tag);
	}
}

// --- Reloading --------------------------------------------------------------------------------

void UZSCombatComponent::StartReload()
{
	const FZSItemStack* Stack = GetEquippedStack();
	const FZSItemRow* Row = GetEquippedRow();
	if (!Stack || !Row || !Row->IsFirearm() || bReloading || Stack->AmmoInMag >= Row->MagazineSize)
	{
		return;
	}

	if (Inventory->CountItem(Row->AmmoItem) <= 0)
	{
		if (AZSCharacter* Owner = Cast<AZSCharacter>(GetOwner()))
		{
			Owner->Notify(FText::FromString(TEXT("No ammo")), FLinearColor(1.f, 0.6f, 0.2f));
		}
		return;
	}

	bReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UZSCombatComponent::FinishReload, FMath::Max(0.1f, Row->ReloadTime), false);
	OnReloadStarted.Broadcast();
}

void UZSCombatComponent::FinishReload()
{
	bReloading = false;

	const FZSItemStack* Stack = GetEquippedStack();
	const FZSItemRow* Row = GetEquippedRow();
	if (!Stack || !Row || !Row->IsFirearm())
	{
		return;
	}

	const int32 Need = Row->MagazineSize - Stack->AmmoInMag;
	const FName AmmoId = Row->AmmoItem;

	// Removing ammo can shift slots (and re-target EquippedSlot), so re-fetch the weapon afterwards.
	const int32 Taken = Inventory->RemoveItem(AmmoId, Need);
	if (FZSItemStack* Weapon = GetEquippedMutable())
	{
		Weapon->AmmoInMag += Taken;
	}
}

void UZSCombatComponent::CancelReload()
{
	if (bReloading)
	{
		bReloading = false;
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	}
}
