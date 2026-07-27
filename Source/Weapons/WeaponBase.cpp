#include "WeaponBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true); // 추가: 물리 드랍 시 움직임 동기화
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	WeaponMesh->SetGenerateOverlapEvents(true);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(RootComponent);
	PickupSphere->SetSphereRadius(PickupSphereRadius);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void AWeaponBase::OnEquipped(ACharacter* NewOwner)
{
	SetOwner(Cast<APawn>(NewOwner));
	SetInstigator(Cast<APawn>(NewOwner));
	SetActorEnableCollision(false);
	SetReplicatingMovement(false); // 장착 중에는 불필요한 독립적 이동 동기화를 꺼서 부착 텔레포트 오류 방지

	if (WeaponMesh) {
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetGenerateOverlapEvents(false);
	}
	if (PickupSphere) {
		PickupSphere->SetGenerateOverlapEvents(false);
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeaponBase::OnUnequipped(bool bDropToWorld)
{
	SetOwner(nullptr);
	SetInstigator(nullptr);
	if (!WeaponMesh) return;
	SetActorEnableCollision(bDropToWorld);
	if (bDropToWorld) {
		SetReplicatingMovement(true); // 바닥에 떨어질 때는 물리 동기화를 위해 다시 켬
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		
		// 튕겨져 날아가는 것을 방지하기 위해 마찰/저항(Damping)을 매우 강하게 설정
		WeaponMesh->SetLinearDamping(5.0f);
		WeaponMesh->SetAngularDamping(5.0f);
		WeaponMesh->SetGenerateOverlapEvents(true);
	}
	else {
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetGenerateOverlapEvents(false);
	}
	if (PickupSphere) {
		const ECollisionEnabled::Type NewEnabled = bDropToWorld ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
		PickupSphere->SetCollisionEnabled(NewEnabled);
		PickupSphere->SetGenerateOverlapEvents(bDropToWorld);
	}
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnPickupBegin);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnPickupEnd);
	}
}
void AWeaponBase::OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor))
	{
		QPCharacter->SetOverlappingWeapon(this);
	}
}
void AWeaponBase::OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor))
	{
		if (QPCharacter->GetOverlappingWeapon() == this)
		{
			QPCharacter->SetOverlappingWeapon(nullptr);
		}
	}
}


void AWeaponBase::StartFire_Implementation()
{
}

void AWeaponBase::StopAttack_Implementation()
{
}
