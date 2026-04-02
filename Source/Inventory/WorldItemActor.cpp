#include "WorldItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Components/SkeletalMeshComponent.h"
AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ItemData = nullptr;
	Quantity = 1;
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(Root);
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(true);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ItemMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(Root);
	PickupSphere->SetSphereRadius(PickupSphereRadius);

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 쿼리 전용 충돌 설정
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널 무시
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 플레이어와 오버랩 설정
	PickupSphere->SetGenerateOverlapEvents(true); // 오버랩 이벤트 생성 활성화



}

void AWorldItemActor::OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor))
	{
		QPCharacter->SetOverlappingWorldItem(this);
	}
}

void AWorldItemActor::OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor))
	{
		if (QPCharacter->GetOverlappingWorldItem() == this)
		{
			QPCharacter->SetOverlappingWorldItem(nullptr);
		}
	}
}

void AWorldItemActor::BeginPlay()
{
	Super::BeginPlay();
	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWorldItemActor::OnPickupBegin);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWorldItemActor::OnPickupEnd);
	}
}
