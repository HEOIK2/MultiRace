#include "CXCoinItem.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACXCoinItem::ACXCoinItem()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CoinMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"));
	if (CoinMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CoinMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CoinMaterial(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	if (CoinMaterial.Succeeded())
	{
		MeshComponent->SetMaterial(0, CoinMaterial.Object);
	}

	MeshComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
	MeshComponent->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.15f));
}

void ACXCoinItem::ApplyItemEffect(ACXPawn* Vehicle)
{
	if (ACXPlayerState* CXPlayerState = Vehicle->GetPlayerState<ACXPlayerState>())
	{
		CXPlayerState->AddCoins(CoinAmount);
	}
}
