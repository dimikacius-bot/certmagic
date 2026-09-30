#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

enum class EZSBasicShape : uint8
{
	Cube,
	Cylinder,
	Sphere
};

/**
 * Engine-primitive stand-ins so gameplay actors are visible before any art is imported.
 * Replace by assigning real meshes in Blueprint subclasses.
 */
namespace ZSPlaceholder
{
	/** Call from a constructor only (uses ConstructorHelpers). */
	void SetShape(UStaticMeshComponent* Comp, EZSBasicShape Shape);

	/** Tints the primitive. Safe at runtime and in OnConstruction. */
	void SetColor(UStaticMeshComponent* Comp, const FLinearColor& Color);
}
