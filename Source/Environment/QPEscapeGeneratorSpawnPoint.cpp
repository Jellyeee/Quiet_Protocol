#include "PJ_Quiet_Protocol/Environment/QPEscapeGeneratorSpawnPoint.h"

#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

AQPEscapeGeneratorSpawnPoint::AQPEscapeGeneratorSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	BillboardComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComp"));
	SetRootComponent(BillboardComp);
	BillboardComp->bHiddenInGame = true;
}

void AQPEscapeGeneratorSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
}
