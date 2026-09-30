#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZSInteractable.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UZSInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can look at and press [E] on. The actor must block the Visibility
 * trace channel for the interaction sweep to find it.
 */
class ZOMBIESURVIVAL_API IZSInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(const AActor* Interactor) const { return true; }
	virtual FText GetInteractPrompt(const AActor* Interactor) const = 0;
	virtual void Interact(AActor* Interactor) = 0;
};
