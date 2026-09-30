#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/ZSInteractable.h"
#include "ZSMerchantNPC.generated.h"

class UZSMerchantComponent;
class UZSInventoryComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EZSMerchantType : uint8
{
	/** Buys most everyday goods, sells food, bandages and basic melee weapons. */
	General,
	/** Buys and sells weapons and ammo. Deep pockets. */
	Gunsmith,
	/** Buys and sells medical supplies and food. */
	Medic,
	/** Buys anything, pays well for trade goods, sells jewelry and electronics at a steep markup. */
	Fence,
	/** No defaults applied: configure everything on the Merchant component yourself. */
	Custom
};

/**
 * A trader NPC. Drop it inside an AZSSafeZone, pick a type and you are done. Subclass in Blueprint to
 * give it a real skeletal mesh and idle animation, or to hand-tune stock, prices and cash.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSMerchantNPC : public ACharacter, public IZSInteractable
{
	GENERATED_BODY()

public:
	AZSMerchantNPC();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	EZSMerchantType MerchantType = EZSMerchantType::General;

	/** Leave empty to use the archetype's default name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Merchant")
	TObjectPtr<UZSMerchantComponent> Merchant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Merchant")
	TObjectPtr<UZSInventoryComponent> Stock;

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

	// IZSInteractable
	virtual FText GetInteractPrompt(const AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;

private:
	void ApplyArchetype();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> NameLabel;
};
