#include "Player/ZSCharacter.h"
#include "Core/ZSGameMode.h"
#include "Core/ZSInteractable.h"
#include "Core/ZSSettings.h"
#include "Economy/ZSPickup.h"
#include "Economy/ZSSafeZone.h"
#include "Inventory/ZSItemSubsystem.h"
#include "Player/ZSPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationInvokerComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"

AZSCharacter::AZSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(10.f, 0.f, 60.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// No first-person arms yet: hide the body mesh from its own camera.
	GetMesh()->SetOwnerNoSee(true);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->JumpZVelocity = 480.f;
	Movement->AirControl = 0.35f;
	Movement->BrakingDecelerationWalking = 2200.f;
	Movement->CrouchedHalfHeight = 60.f;
	Movement->NavAgentProps.bCanCrouch = true;

	Inventory = CreateDefaultSubobject<UZSInventoryComponent>(TEXT("Inventory"));
	Health = CreateDefaultSubobject<UZSHealthComponent>(TEXT("Health"));
	Combat = CreateDefaultSubobject<UZSCombatComponent>(TEXT("Combat"));

	// Huge maps: the navmesh only exists around the player, and the spawner keeps zombies inside it.
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(7000.f, 9000.f);

	// Lets zombies' sight sense see the player.
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());

	StartingItems = {
		FZSItemStack(TEXT("Pipe"), 1),
		FZSItemStack(TEXT("Bandage"), 2),
		FZSItemStack(TEXT("CannedFood"), 1)
	};
}

void AZSCharacter::BeginPlay()
{
	Super::BeginPlay();

	Stamina = MaxStamina;
	Health->OnDeath.AddDynamic(this, &AZSCharacter::HandleDeath);
	Health->OnHealthChanged.AddDynamic(this, &AZSCharacter::HandleHealthChanged);
	LastHealth = Health->GetHealth();

	Inventory->SetMoney(StartingMoney);
	for (const FZSItemStack& Item : StartingItems)
	{
		Inventory->AddStack(Item);
	}
	Combat->AutoEquipBest();
}

// --- Input ------------------------------------------------------------------------------------

void AZSCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxis("MoveForward", this, &AZSCharacter::MoveForward);
	Input->BindAxis("MoveRight", this, &AZSCharacter::MoveRight);
	Input->BindAxis("Turn", this, &AZSCharacter::Turn);
	Input->BindAxis("LookUp", this, &AZSCharacter::LookUp);

	Input->BindAction("Jump", IE_Pressed, this, &AZSCharacter::JumpPressed);
	Input->BindAction("Jump", IE_Released, this, &AZSCharacter::JumpReleased);
	Input->BindAction("Sprint", IE_Pressed, this, &AZSCharacter::SprintPressed);
	Input->BindAction("Sprint", IE_Released, this, &AZSCharacter::SprintReleased);
	Input->BindAction("Crouch", IE_Pressed, this, &AZSCharacter::CrouchPressed);
	Input->BindAction("Crouch", IE_Released, this, &AZSCharacter::CrouchReleased);

	Input->BindAction("Attack", IE_Pressed, this, &AZSCharacter::AttackPressed);
	Input->BindAction("Attack", IE_Released, this, &AZSCharacter::AttackReleased);
	Input->BindAction("Reload", IE_Pressed, this, &AZSCharacter::ReloadPressed);
	Input->BindAction("NextWeapon", IE_Pressed, this, &AZSCharacter::NextWeaponPressed);
	Input->BindAction("PrevWeapon", IE_Pressed, this, &AZSCharacter::PrevWeaponPressed);
	Input->BindAction("Holster", IE_Pressed, this, &AZSCharacter::HolsterPressed);
	Input->BindAction("UseHealing", IE_Pressed, this, &AZSCharacter::HealPressed);

	Input->BindAction("Interact", IE_Pressed, this, &AZSCharacter::InteractPressed);
	Input->BindAction("ToggleInventory", IE_Pressed, this, &AZSCharacter::ToggleInventoryPressed);
}

void AZSCharacter::MoveForward(float Value)
{
	if (Value != 0.f && !bDead && !bMantling)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AZSCharacter::MoveRight(float Value)
{
	if (Value != 0.f && !bDead && !bMantling)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AZSCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AZSCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AZSCharacter::JumpPressed()
{
	bJumpHeld = true;
	if (bDead || IsUIBlocking() || bMantling)
	{
		return;
	}

	// Jumping into a ledge grabs it; otherwise it is a normal jump.
	if (TryStartMantle())
	{
		return;
	}
	if (CanJump() && TrySpendStamina(JumpStaminaCost))
	{
		Jump();
	}
}

void AZSCharacter::JumpReleased()
{
	bJumpHeld = false;
	StopJumping();
}

void AZSCharacter::CrouchPressed()
{
	if (!bDead && !bMantling)
	{
		Crouch();
	}
}

void AZSCharacter::CrouchReleased()
{
	UnCrouch();
}

void AZSCharacter::AttackPressed()
{
	if (bDead || IsUIBlocking())
	{
		return;
	}
	bAttackHeld = true;
	Combat->PrimaryAttack();
}

void AZSCharacter::ReloadPressed()
{
	if (!bDead && !IsUIBlocking())
	{
		Combat->StartReload();
	}
}

void AZSCharacter::InteractPressed()
{
	if (bDead)
	{
		return;
	}

	if (AZSPlayerController* PC = Cast<AZSPlayerController>(GetController()))
	{
		if (PC->IsTrading())
		{
			PC->CloseTrade();
			return;
		}
	}

	if (IZSInteractable* Interactable = Cast<IZSInteractable>(FocusedActor.Get()))
	{
		Interactable->Interact(this);
	}
}

void AZSCharacter::HealPressed()
{
	if (!bDead && !IsUIBlocking())
	{
		UseHealingItem();
	}
}

void AZSCharacter::NextWeaponPressed()
{
	if (!bDead && !IsUIBlocking())
	{
		Combat->CycleWeapon(+1);
	}
}

void AZSCharacter::PrevWeaponPressed()
{
	if (!bDead && !IsUIBlocking())
	{
		Combat->CycleWeapon(-1);
	}
}

void AZSCharacter::HolsterPressed()
{
	if (!bDead)
	{
		Combat->Unequip();
	}
}

void AZSCharacter::ToggleInventoryPressed()
{
	if (AZSPlayerController* PC = Cast<AZSPlayerController>(GetController()))
	{
		PC->ToggleInventory();
	}
}

bool AZSCharacter::IsUIBlocking() const
{
	const AZSPlayerController* PC = Cast<AZSPlayerController>(GetController());
	return PC && PC->IsTrading();
}

// --- Tick -------------------------------------------------------------------------------------

void AZSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	if (bMantling)
	{
		TickMantle(DeltaSeconds);
		return;
	}

	UpdateSprintAndStamina(DeltaSeconds);

	// Camera eye height eases down when crouching.
	const float TargetEye = bIsCrouched ? 38.f : 60.f;
	FVector CameraLocation = FirstPersonCamera->GetRelativeLocation();
	CameraLocation.Z = FMath::FInterpTo(CameraLocation.Z, TargetEye, DeltaSeconds, 12.f);
	FirstPersonCamera->SetRelativeLocation(CameraLocation);

	// Holding jump against a wall while airborne still grabs the ledge.
	if (bJumpHeld && GetCharacterMovement()->IsFalling() && Now >= NextMantleProbeTime)
	{
		NextMantleProbeTime = Now + 0.06f;
		TryStartMantle();
	}

	if (bAttackHeld && !IsUIBlocking())
	{
		Combat->PrimaryAttack();
	}

	FocusTimer -= DeltaSeconds;
	if (FocusTimer <= 0.f)
	{
		FocusTimer = 0.1f;
		UpdateFocus();
	}
}

void AZSCharacter::UpdateSprintAndStamina(float DeltaSeconds)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Now = GetWorld()->GetTimeSeconds();

	const FVector Velocity2D = GetVelocity().GetSafeNormal2D();
	const bool bMovingForward = GetVelocity().SizeSquared2D() > FMath::Square(150.f)
		&& FVector::DotProduct(Velocity2D, GetActorForwardVector()) > 0.5f;

	const bool bSprinting = bWantsSprint && !bExhausted && !bIsCrouched && bMovingForward && Movement->IsMovingOnGround();
	Movement->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;

	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - SprintDrainPerSecond * DeltaSeconds);
		LastStaminaUseTime = Now;
		if (Stamina <= 0.f)
		{
			bExhausted = true;
		}
	}
	else if (Now - LastStaminaUseTime > StaminaRegenDelay)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaSeconds);
		if (bExhausted && Stamina >= ExhaustRecoverAt)
		{
			bExhausted = false;
		}
	}
}

bool AZSCharacter::TrySpendStamina(float Cost)
{
	if (Cost <= 0.f)
	{
		return true;
	}
	// Allow the action when at least a third of its cost is left; you just end up drained.
	if (Stamina < Cost * 0.35f)
	{
		return false;
	}
	Stamina = FMath::Max(0.f, Stamina - Cost);
	LastStaminaUseTime = GetWorld()->GetTimeSeconds();
	return true;
}

// --- Parkour ----------------------------------------------------------------------------------

bool AZSCharacter::TryStartMantle()
{
	UWorld* World = GetWorld();
	if (bMantling || bDead || World->GetTimeSeconds() < NextMantleTime)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Location = GetActorLocation();
	const float FeetZ = Location.Z - HalfHeight;
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();

	// Only static world geometry counts as a ledge (never zombies or NPCs).
	const FCollisionObjectQueryParams Objects(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZSMantle), false, this);

	// 1) Find a wall in front of us, probing from knee height upwards.
	FHitResult Wall;
	bool bFoundWall = false;
	for (float Height = 40.f; Height < MaxMantleHeight - 10.f; Height += 40.f)
	{
		const FVector Start(Location.X, Location.Y, FeetZ + Height);
		const FVector End = Start + Forward * (Radius + MantleReach);
		if (World->LineTraceSingleByObjectType(Wall, Start, End, Objects, Params) && FMath::Abs(Wall.ImpactNormal.Z) < 0.35f)
		{
			bFoundWall = true;
			break;
		}
	}
	if (!bFoundWall)
	{
		return false;
	}

	// 2) Find the top of it by tracing down just behind the wall face.
	const FVector Inward = -Wall.ImpactNormal.GetSafeNormal2D();
	FVector TopStart = Wall.ImpactPoint + Inward * (Radius + 8.f);
	TopStart.Z = FeetZ + MaxMantleHeight + 20.f;
	FVector TopEnd = TopStart;
	TopEnd.Z = FeetZ + 30.f;

	FHitResult Top;
	if (!World->LineTraceSingleByObjectType(Top, TopStart, TopEnd, Objects, Params) || Top.bStartPenetrating || Top.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	const float LedgeHeight = Top.ImpactPoint.Z - FeetZ;
	if (LedgeHeight < MinMantleHeight || LedgeHeight > MaxMantleHeight)
	{
		return false;
	}

	// 3) Is there room to stand there, and is the way up clear?
	const FVector Target(Top.ImpactPoint.X, Top.ImpactPoint.Y, Top.ImpactPoint.Z + HalfHeight + 2.f);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius * 0.9f, HalfHeight * 0.95f);
	if (World->OverlapBlockingTestByChannel(Target, FQuat::Identity, ECC_Pawn, Shape, Params))
	{
		return false;
	}
	if (World->SweepTestByChannel(Location, FVector(Location.X, Location.Y, Target.Z), FQuat::Identity, ECC_Pawn, Shape, Params))
	{
		return false;
	}

	if (!TrySpendStamina(MantleStaminaCost))
	{
		return false;
	}

	// Low obstacles are a quick vault, tall ones a slower climb.
	const float Alpha = FMath::Clamp((LedgeHeight - VaultMaxHeight) / FMath::Max(1.f, MaxMantleHeight - VaultMaxHeight), 0.f, 1.f);
	MantleDuration = FMath::Lerp(0.3f, 0.6f, Alpha);
	MantleElapsed = 0.f;
	MantleStart = Location;
	MantleEnd = Target;
	MantleForward = Forward;
	bMantling = true;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();

	OnMantleStarted(LedgeHeight);
	return true;
}

void AZSCharacter::TickMantle(float DeltaSeconds)
{
	MantleElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(MantleElapsed / MantleDuration, 0.f, 1.f);

	// Rise first, then slide over the edge, so the capsule never cuts through the ledge.
	constexpr float RisePhase = 0.6f;
	FVector Position;
	if (Alpha < RisePhase)
	{
		const float T = FMath::InterpEaseOut(0.f, 1.f, Alpha / RisePhase, 2.f);
		Position = FVector(MantleStart.X, MantleStart.Y, FMath::Lerp(MantleStart.Z, MantleEnd.Z, T));
	}
	else
	{
		const float T = FMath::InterpEaseInOut(0.f, 1.f, (Alpha - RisePhase) / (1.f - RisePhase), 2.f);
		Position = FVector(FMath::Lerp(MantleStart.X, MantleEnd.X, T), FMath::Lerp(MantleStart.Y, MantleEnd.Y, T), MantleEnd.Z);
	}
	SetActorLocation(Position, false, nullptr, ETeleportType::None);

	if (Alpha >= 1.f)
	{
		bMantling = false;
		NextMantleTime = GetWorld()->GetTimeSeconds() + 0.25f;
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity = MantleForward * 250.f; // keep some momentum over the top
	}
}

// --- Interaction / safe zones -----------------------------------------------------------------

void AZSCharacter::UpdateFocus()
{
	AActor* NewFocus = nullptr;

	if (Controller)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ZSFocus), false, this);
		const FVector End = ViewLocation + ViewRotation.Vector() * InteractRange;
		if (GetWorld()->SweepSingleByChannel(Hit, ViewLocation, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(20.f), Params))
		{
			AActor* Candidate = Hit.GetActor();
			const IZSInteractable* Interactable = Cast<IZSInteractable>(Candidate);
			if (Interactable && Interactable->CanInteract(this))
			{
				NewFocus = Candidate;
			}
		}
	}

	FocusedActor = NewFocus;
}

FText AZSCharacter::GetFocusPrompt() const
{
	if (const IZSInteractable* Interactable = Cast<IZSInteractable>(FocusedActor.Get()))
	{
		return Interactable->GetInteractPrompt(this);
	}
	return FText::GetEmpty();
}

bool AZSCharacter::IsInSafeZone() const
{
	return GetCurrentSafeZone() != nullptr;
}

AZSSafeZone* AZSCharacter::GetCurrentSafeZone() const
{
	for (const TWeakObjectPtr<AZSSafeZone>& Zone : SafeZones)
	{
		if (Zone.IsValid())
		{
			return Zone.Get();
		}
	}
	return nullptr;
}

void AZSCharacter::EnterSafeZone(AZSSafeZone* Zone)
{
	const bool bWasSafe = IsInSafeZone();
	SafeZones.AddUnique(Zone);
	if (!bWasSafe && Zone)
	{
		Notify(FText::FromString(FString::Printf(TEXT("Entered %s"), *Zone->ZoneName.ToString())), FLinearColor(0.4f, 1.f, 0.5f));
	}
}

void AZSCharacter::ExitSafeZone(AZSSafeZone* Zone)
{
	SafeZones.Remove(Zone);
	SafeZones.RemoveAll([](const TWeakObjectPtr<AZSSafeZone>& Z) { return !Z.IsValid(); });
	if (!IsInSafeZone())
	{
		Notify(FText::FromString(TEXT("Left the safe zone - stay alert")), FLinearColor(1.f, 0.7f, 0.3f));
	}
}

void AZSCharacter::Notify(const FText& Message, const FLinearColor& Color)
{
	if (AZSPlayerController* PC = Cast<AZSPlayerController>(GetController()))
	{
		PC->PushNotification(Message, Color);
	}
}

// --- Healing / damage / death -----------------------------------------------------------------

bool AZSCharacter::UseHealingItem()
{
	const UZSItemSubsystem* Items = UZSItemSubsystem::Get(this);
	if (!Items || bDead)
	{
		return false;
	}

	const float Missing = Health->GetMaxHealth() - Health->GetHealth();
	if (Missing < 1.f)
	{
		Notify(FText::FromString(TEXT("Already at full health")), FLinearColor(0.8f, 0.8f, 0.8f));
		return false;
	}

	// Prefer the item whose heal amount best matches what is missing, so a Medkit is not wasted on a scratch.
	int32 BestSlot = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();
	const TArray<FZSItemStack>& Slots = Inventory->GetSlots();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FZSItemRow* Row = Items->Find(Slots[Index].ItemId);
		if (Row && Row->HealAmount > 0.f)
		{
			const float Score = FMath::Abs(Row->HealAmount - Missing);
			if (Score < BestScore)
			{
				BestScore = Score;
				BestSlot = Index;
			}
		}
	}

	if (BestSlot == INDEX_NONE)
	{
		Notify(FText::FromString(TEXT("Nothing to heal with")), FLinearColor(1.f, 0.6f, 0.2f));
		return false;
	}

	const FZSItemRow* Row = Items->Find(Slots[BestSlot].ItemId);
	const float Amount = Row->HealAmount;
	const FText Name = Row->DisplayName;

	FZSItemStack Used;
	Inventory->RemoveFromSlot(BestSlot, 1, Used);
	const float Healed = Health->Heal(Amount);
	Notify(FText::FromString(FString::Printf(TEXT("Used %s (+%d HP)"), *Name.ToString(), FMath::RoundToInt(Healed))), FLinearColor(0.5f, 1.f, 0.5f));
	return true;
}

void AZSCharacter::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	if (LastHealth >= 0.f && NewHealth < LastHealth)
	{
		LastDamageTime = GetWorld()->GetTimeSeconds();
	}
	LastHealth = NewHealth;
}

float AZSCharacter::GetSecondsSinceDamage() const
{
	return GetWorld()->GetTimeSeconds() - LastDamageTime;
}

void AZSCharacter::HandleDeath(AActor* Killer)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	bAttackHeld = false;
	bMantling = false;
	GetCharacterMovement()->DisableMovement();

	// Part of your cash stays where you fell. Go back for it.
	const int32 Lost = FMath::FloorToInt(Inventory->GetMoney() * UZSSettings::Get()->DeathCashPenalty);
	if (Lost > 0 && Inventory->TrySpendMoney(Lost))
	{
		AZSPickup::SpawnCash(GetWorld(), GetActorLocation(), Lost, 1800.f);
		Notify(FText::FromString(FString::Printf(TEXT("You dropped $%d where you fell"), Lost)), FLinearColor(1.f, 0.4f, 0.3f));
	}

	OnDied();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AZSCharacter::Respawn, RespawnDelay, false);
}

void AZSCharacter::Respawn()
{
	FTransform Spot = GetActorTransform();
	if (const AZSGameMode* GameMode = GetWorld()->GetAuthGameMode<AZSGameMode>())
	{
		Spot = GameMode->GetRespawnTransform(GetController());
	}

	TeleportTo(Spot.GetLocation(), Spot.Rotator(), false, true);
	if (Controller)
	{
		Controller->SetControlRotation(Spot.Rotator());
	}

	Health->Revive(0.6f);
	Stamina = MaxStamina;
	bExhausted = false;
	bDead = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Notify(FText::FromString(TEXT("You made it back to safety")), FLinearColor(0.6f, 0.9f, 1.f));
}
