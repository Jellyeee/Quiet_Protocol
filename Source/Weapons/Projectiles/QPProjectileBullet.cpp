#include "QPProjectileBullet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
AQPProjectileBullet::AQPProjectileBullet()
{
	PrimaryActorTick.bCanEverTick = true;
	BulletCollision = CreateDefaultSubobject<USphereComponent>(TEXT("BulletCollision")); //총알 충돌 컴포넌트 생성
	SetRootComponent(BulletCollision); //루트 컴포넌트로 설정
	BulletCollision->InitSphereRadius(5.f); //충돌 반지름 설정
	BulletCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //충돌 활성화 설정
	BulletCollision->SetCollisionResponseToAllChannels(ECR_Block); //모든 채널에 대해 충돌 응답 설정

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")); //총알 이동 컴포넌트 생성
	ProjectileMovement->bRotationFollowsVelocity = true; //회전이 속도를 따르도록 설정
	ProjectileMovement->ProjectileGravityScale = 1.0f; //중력 스케일 설정

	//총알 메쉬 컴포넌트 생성
	BulletMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(RootComponent); //루트 컴포넌트에
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //충돌 비활성화 설정
	InitialLifeSpan = 5.0f; //수명 설정
}

void AQPProjectileBullet::SetBulletVelocity(const FVector& Direction, float Speed)
{
	if (ProjectileMovement) {
		const FVector CommonVelocity = Direction.GetSafeNormal() * Speed; //방향을 정규화하고 속도 곱하기
		ProjectileMovement->Velocity = CommonVelocity; //총알 이동 컴포넌트
		ProjectileMovement->InitialSpeed = Speed; //초기 속도 설정
		ProjectileMovement->MaxSpeed = Speed; //최대 속도 설정
	}
}

void AQPProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	BulletCollision->OnComponentHit.AddDynamic(this, &AQPProjectileBullet::OnHit);
	PrevLocation = GetActorLocation(); //이전 위치 초기화
	if (AActor* OwnerActor = GetOwner())
	{
		//총알이 소유자와 충돌하지 않도록 설정
		BulletCollision->IgnoreActorWhenMoving(OwnerActor, true); //소유자 무시
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		//총알이 인스티게이터와 충돌하지 않도록 설정
		BulletCollision->IgnoreActorWhenMoving(InstigatorPawn, true); //인스티게이터 무시
	}

	if (TrailFX) {
		TrailFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailFX, RootComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true, true, ENCPoolMethod::AutoRelease, true);  //트레일 이펙트 생성 및 부착
	}
	if (TrailFXComponent) {
		TrailFXComponent->SetWorldScale3D(TrailFXScale);  //트레일 이펙트 스케일 설정
		bDebugDrawTracer = false;
	}
}

void AQPProjectileBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bDebugDrawTracer) {
		const FVector CurrentLocation = GetActorLocation(); //현재 위치 가져오기
		DrawDebugLine(GetWorld(), PrevLocation, CurrentLocation, FColor::Green, false, DebugSegmentLifeTime, 0, DebugThickness); //디버그 선 그리기
		PrevLocation = CurrentLocation; //이전 위치 업데이트
	}
}

void AQPProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AActor* MyOwner = GetOwner();

	if (OtherActor && OtherActor != this && OtherActor != MyOwner)
	{
		const FVector Dir = GetVelocity().GetSafeNormal();
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, Dir, Hit, GetInstigatorController(), this, DamageTypeClass);
	}
	Destroy();
}


