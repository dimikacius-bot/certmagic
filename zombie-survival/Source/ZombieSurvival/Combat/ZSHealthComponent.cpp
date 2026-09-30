#include "Combat/ZSHealthComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"

UZSHealthComponent::UZSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Health < 0.f)
	{
		Health = MaxHealth;
	}

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UZSHealthComponent::HandleAnyDamage);
	}
}

void UZSHealthComponent::SetMaxHealth(float NewMax)
{
	MaxHealth = FMath::Max(1.f, NewMax);
	Health = MaxHealth;
	bDead = false;
}

void UZSHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bDead || Damage <= 0.f)
	{
		return;
	}

	Health = FMath::Clamp(GetHealth() - Damage, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		bDead = true;
		AActor* Killer = InstigatedBy && InstigatedBy->GetPawn() ? static_cast<AActor*>(InstigatedBy->GetPawn()) : DamageCauser;
		OnDeath.Broadcast(Killer);
	}
}

float UZSHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Before = GetHealth();
	Health = FMath::Min(MaxHealth, Before + Amount);
	OnHealthChanged.Broadcast(Health, MaxHealth);
	return Health - Before;
}

void UZSHealthComponent::Revive(float HealthFraction)
{
	bDead = false;
	Health = FMath::Clamp(MaxHealth * HealthFraction, 1.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}
