#include "CXSpeedBuffItem.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACXSpeedBuffItem::ACXSpeedBuffItem()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BuffMesh(TEXT("/Game/StarterContent/Shapes/Shape_QuadPyramid.Shape_QuadPyramid"));
	if (BuffMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(BuffMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BuffMaterial(TEXT("/Game/StarterContent/Materials/M_Tech_Hex_Tile_Pulse.M_Tech_Hex_Tile_Pulse"));
	if (BuffMaterial.Succeeded())
	{
		MeshComponent->SetMaterial(0, BuffMaterial.Object);
	}

	MeshComponent->SetRelativeScale3D(FVector(0.7f));
	DestroyEffectScale = FVector(0.5f);
}

void ACXSpeedBuffItem::ApplyItemEffect(ACXPawn* Vehicle)
{
	if (ACXPlayerState* CXPlayerState = Vehicle->GetPlayerState<ACXPlayerState>())
	{
		CXPlayerState->AddBonusMaxSpeed(BonusMaxSpeedAmount);
	}
}
