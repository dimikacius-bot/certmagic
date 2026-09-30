#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inventory/ZSItemTypes.h"
#include "ZSItemSubsystem.generated.h"

/** Item catalog. Built-ins are registered in code so the game runs with zero imported assets. */
UCLASS()
class ZOMBIESURVIVAL_API UZSItemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UZSItemSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FZSItemRow* Find(FName ItemId) const { return Items.Find(ItemId); }
	const TMap<FName, FZSItemRow>& GetAll() const { return Items; }

	/** Display name, or the raw id when unknown. */
	FText GetDisplayName(FName ItemId) const;

private:
	void RegisterBuiltins();

	TMap<FName, FZSItemRow> Items;
};
