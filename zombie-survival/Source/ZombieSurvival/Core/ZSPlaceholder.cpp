#include "Core/ZSPlaceholder.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

void ZSPlaceholder::SetShape(UStaticMeshComponent* Comp, EZSBasicShape Shape)
{
	if (!Comp)
	{
		return;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	switch (Shape)
	{
	case EZSBasicShape::Cube:
		if (Cube.Succeeded()) { Comp->SetStaticMesh(Cube.Object); }
		break;
	case EZSBasicShape::Cylinder:
		if (Cylinder.Succeeded()) { Comp->SetStaticMesh(Cylinder.Object); }
		break;
	case EZSBasicShape::Sphere:
		if (Sphere.Succeeded()) { Comp->SetStaticMesh(Sphere.Object); }
		break;
	}

	if (Material.Succeeded())
	{
		Comp->SetMaterial(0, Material.Object);
	}
}

void ZSPlaceholder::SetColor(UStaticMeshComponent* Comp, const FLinearColor& Color)
{
	if (!Comp)
	{
		return;
	}
	if (UMaterialInstanceDynamic* MID = Comp->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
