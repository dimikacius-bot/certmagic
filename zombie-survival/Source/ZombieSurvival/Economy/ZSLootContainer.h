#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ZSInteractable.h"
#include "Economy/ZSLootTables.h"
#include "ZSLootContainer.generated.h"

class UStaticMeshComponent;

/**
 * A searchable crate / locker / cabinet. Drop these in the level (or spawn a Blueprint subclass from a
 * PCG graph on building interiors); pick the loot table that fits the place. Refills after a few days.
 */
UCLASS()
class ZOMBIESURVIVAL_API AZSLootContainer : public AActor, public IZSInteractable
{
	GENERATED_BODY()

public:
	AZSLootContainer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EZSLootTable LootTable = EZSLootTable::Residential;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FText ContainerName = FText::FromString(TEXT("Crate"));

	/** In-game days before a looted container is refilled. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	int32 RefillAfterDays = 3;

	// IZSInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual FText GetInteractPrompt(const AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleNewDay(int32 DayNumber);

	void UpdateVisual();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	bool bLooted = false;
	int32 LootedOnDay = 0;
};
