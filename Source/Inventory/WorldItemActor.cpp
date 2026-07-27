#include "WorldItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ItemData = nullptr;
	Quantity = 1;
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(Root);
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(true);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 물리 충돌 무시
	ItemMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 카메라 줌인 방지
	
	// 물리 활성화 시 튕겨져 날아가는 것을 방지하기 위해 마찰/저항(Damping) 추가
	ItemMesh->SetLinearDamping(2.0f);
	ItemMesh->SetAngularDamping(2.0f);

	ItemSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemSkeletalMesh"));
	ItemSkeletalMesh->SetupAttachment(Root);
	ItemSkeletalMesh->SetSimulatePhysics(false);
	ItemSkeletalMesh->SetEnableGravity(true);
	ItemSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 기본적으로는 꺼둠
	ItemSkeletalMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ItemSkeletalMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ItemSkeletalMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); 
	ItemSkeletalMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); 
	ItemSkeletalMesh->SetLinearDamping(2.0f);
	ItemSkeletalMesh->SetAngularDamping(2.0f);
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
	UpdateItemMesh();
}

void AWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateItemMesh();
}

void AWorldItemActor::OnRep_ItemData()
{
	UpdateItemMesh();
}

void AWorldItemActor::UpdateItemMesh()
{
	if (!ItemData) return;

	// 스켈레탈 메쉬가 설정되어 있으면 스켈레탈 메쉬 사용 (방어복 등)
	if (ItemData->PickupSkeletalMesh && ItemSkeletalMesh)
	{
		ItemSkeletalMesh->SetSkeletalMesh(ItemData->PickupSkeletalMesh);
		ItemSkeletalMesh->SetVisibility(true);
		ItemSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (ItemMesh)
		{
			ItemMesh->SetVisibility(false);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	// 그 외 일반 스태틱 메쉬 사용
	else if (ItemData->PickupMesh && ItemMesh)
	{
		ItemMesh->SetStaticMesh(ItemData->PickupMesh);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (ItemSkeletalMesh)
		{
			ItemSkeletalMesh->SetVisibility(false);
			ItemSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AWorldItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWorldItemActor, ItemData);
	DOREPLIFETIME(AWorldItemActor, Quantity);
	DOREPLIFETIME(AWorldItemActor, AssignedSlotIndex);
	DOREPLIFETIME(AWorldItemActor, AssignedCodeNumber);
}
