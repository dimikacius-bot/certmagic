#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ZSInteractable.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * Loose loot in the world. Cash is collected by walking over it; items need [E].
 * Used for dropped items, zombie drops and the cash you lose when you die.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSPickup : public AActor, public IZSInteractable
{
	GENERATED_BODY()

public:
	AZSPickup();

	virtual void Tick(float DeltaSeconds) override;

	// IZSInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual FText GetInteractPrompt(const AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;

	/** Set what this pickup holds. Cash > 0 makes it a cash pickup. */
	void Configure(const FZSItemStack& InItem, int32 InCash);

	/** Spawns a pickup on the ground below Location. */
	static AZSPickup* SpawnItem(UWorld* World, const FVector& Location, const FZSItemStack& Item, float LifeSeconds = 600.f);
	static AZSPickup* SpawnCash(UWorld* World, const FVector& Location, int32 Amount, float LifeSeconds = 900.f);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	FZSItemStack Item;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	int32 Cash = 0;

	bool bCollected = false;
};
