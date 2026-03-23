#include "WeaponBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 리플리케이션 활성화 (클라이언트에서도 무기가 보이도록)
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh")); //무기 메쉬 컴포넌트 생성
	SetRootComponent(WeaponMesh); //RootComponent로 설정

	WeaponMesh->SetSimulatePhysics(false); //물리 시뮬레이션 비활성화
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //충돌 활성화
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore); //모든 채널에 대한 충돌 응답 무시
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); //충돌 채널 설정

	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block); //WorldStatic 채널에 대해 차단 응답 설정
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block); //WorldStatic 채널에 대해 차단 응답 설정
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); //Visibility 채널에 대해 차단 응답 설정
	WeaponMesh->SetGenerateOverlapEvents(true); //겹침 이벤트 생성 활성화

	//PickupSphere 컴포넌트 생성 및 설정
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(RootComponent); //루트 컴포넌트
	PickupSphere->SetSphereRadius(PickupSphereRadius); //픽업 스피어 반지름 설정
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); //쿼리	전용 충돌 설정
	PickupSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //모든 채널에 대한 충돌 응답 무시
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true); //겹침 이벤트 생성 활성화
}

void AWeaponBase::OnEquipped(ACharacter* NewOwner)
{
	SetOwner(Cast<APawn>(NewOwner)); //소유자 설정
	SetInstigator(Cast<APawn>(NewOwner)); //인스티게이터 설정
	SetActorEnableCollision(false); //액터 충돌 비활성화

	if (WeaponMesh) {
		WeaponMesh->SetSimulatePhysics(false); //물리 시뮬레이션 비활성화
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 비활성화
		WeaponMesh->SetGenerateOverlapEvents(false); //겹침 이벤트 비활성화
	}
	if (PickupSphere) {
		PickupSphere->SetGenerateOverlapEvents(false); //겹침 이벤트 비활성화
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 비활성화
	}

}

void AWeaponBase::OnUnequipped(bool bDropToWorld)
{
	SetOwner(nullptr); //소유자 해제
	SetInstigator(nullptr); //인스티게이터 해제
	if (!WeaponMesh) return; //무기 메쉬가 유효하지 않으면 반환
	SetActorEnableCollision(bDropToWorld); //액터 충돌 설정
	if (bDropToWorld) {
		WeaponMesh->SetSimulatePhysics(true); //물리 시뮬레이션 활성화
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //충돌 활성화
		WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); //충돌 채널 설정
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //모든 채널에 대한 충돌 응답 무시
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block); //WorldStatic 채널에 대해 차단 응답 설정
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block); //WorldStatic 채널에 대해 차단 응답 설정
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);//Visibility 채널에 대해 차단 응답 설정
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); //Pawn 채널에 대해 겹침 응답 설정
		WeaponMesh->SetGenerateOverlapEvents(true); //겹침 이벤트 생성 활성화
	}
	else {
		WeaponMesh->SetSimulatePhysics(false); //물리 시뮬레이션 비활성화
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 비활성화
		WeaponMesh->SetGenerateOverlapEvents(false); //겹침 이벤트 비활성화
	}
	if (PickupSphere) {
		const ECollisionEnabled::Type NewEnabled = bDropToWorld ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision; //새 충돌 설정 결정
		PickupSphere->SetCollisionEnabled(NewEnabled); //충돌 설정
		PickupSphere->SetGenerateOverlapEvents(bDropToWorld); //겹침 이벤트 설정
	}
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnPickupBegin); //픽업 시작 오버랩 이벤트 바인딩
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnPickupEnd); //픽업 종료 오버랩 이벤트 바인딩
	}
}
void AWeaponBase::OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor)) //겹친 액터가 QPCharacter인지 확인
	{
		QPCharacter->SetOverlappingWeapon(this); //겹치는 무기 설정
	}
}
void AWeaponBase::OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor))
	{
		if (QPCharacter->GetOverlappingWeapon() == this) //겹치는 무기가 현재 무기인지 확인
		{
			QPCharacter->SetOverlappingWeapon(nullptr); //겹치는 무기 해제
		}
	}
}


void AWeaponBase::StartFire_Implementation()
{
}

void AWeaponBase::StopAttack_Implementation()
{
}
