#include "QPProjectileBullet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
AQPProjectileBullet::AQPProjectileBullet()
{
	PrimaryActorTick.bCanEverTick = true;
	BulletCollision = CreateDefaultSubobject<USphereComponent>(TEXT("BulletCollision"));
	SetRootComponent(BulletCollision);
	BulletCollision->InitSphereRadius(5.f);
	BulletCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BulletCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BulletCollision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 1.0f;

	BulletMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(RootComponent);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InitialLifeSpan = 3.0f;

	bReplicates = true;
	SetReplicateMovement(true);
}

void AQPProjectileBullet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AQPProjectileBullet, ReplicatedInitialVelocity);
}

void AQPProjectileBullet::SetBulletVelocity(const FVector& Direction, float Speed)
{
	if (ProjectileMovement) {
		const FVector CommonVelocity = Direction.GetSafeNormal() * Speed;
		ProjectileMovement->Velocity = CommonVelocity;
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;

		/** [Network] 서버에서 생성된 초기 유속을 RepNotify 변수에 저장하여 클라이언트에 전달 */
		if (HasAuthority())
		{
			ReplicatedInitialVelocity = CommonVelocity;
		}
	}
}

void AQPProjectileBullet::OnRep_InitialVelocity()
{
	/** 클라이언트에서 서버가 보낸 초기 속도 값을 수신했을 때, 자신의 무브먼트 컴포넌트에 적용 */
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = ReplicatedInitialVelocity;
		ProjectileMovement->InitialSpeed = ReplicatedInitialVelocity.Size();
		ProjectileMovement->MaxSpeed = ProjectileMovement->InitialSpeed;
	}
}

void AQPProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	BulletCollision->OnComponentHit.AddDynamic(this, &AQPProjectileBullet::OnHit);
	PrevLocation = GetActorLocation(); //이전 위치 초기화
	if (AActor* OwnerActor = GetOwner())
	{
		/** 
		 * 발사 직후 총알이 발사한 캐릭터(Owner)의 캡슐이나 메시와 충돌하여 
		 * 제자리에서 터지는 것을 방지하기 위해 양방향으로 충돌을 무시하도록 설정합니다.
		 */
		BulletCollision->IgnoreActorWhenMoving(OwnerActor, true); 
		TArray<UPrimitiveComponent*> OwnerComps;
		OwnerActor->GetComponents(OwnerComps);
		for (UPrimitiveComponent* Comp : OwnerComps)
		{
			BulletCollision->IgnoreComponentWhenMoving(Comp, true);
			Comp->IgnoreComponentWhenMoving(BulletCollision, true); 
		}
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		BulletCollision->IgnoreActorWhenMoving(InstigatorPawn, true); 
	}

	/** 
	 * [샷건 발사 로직 보완]
	 * 샷건처럼 여러 발이 동시에 나갈 때, 총알끼리 서로 부딪혀 공중에서 멈추는 현상을 방지합니다.
	 * 현재 월드에 존재하는 모든 투사체를 찾아 서로 무시하도록 설정합니다.
	 */
	TArray<AActor*> FoundBullets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQPProjectileBullet::StaticClass(), FoundBullets);
	for (AActor* BulletActor : FoundBullets)
	{
		if (BulletActor != this)
		{
			BulletCollision->IgnoreActorWhenMoving(BulletActor, true);
			
			if (UPrimitiveComponent* OtherCollision = Cast<UPrimitiveComponent>(BulletActor->GetRootComponent()))
			{
				OtherCollision->IgnoreActorWhenMoving(this, true);
			}
		}
	}

	if (TrailFX) {
		TrailFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailFX, RootComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true, true, ENCPoolMethod::AutoRelease, true);
	}
	if (TrailFXComponent) {
		TrailFXComponent->SetWorldScale3D(TrailFXScale);
	}
}

void AQPProjectileBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AQPProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/** 
	 * [타격 유효성 검사]
	 * 타겟이 유효하지 않거나, 자기 자신/소유자/동일 클래스(다른 총알)일 경우 타격 처리를 하지 않습니다.
	 */
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor->IsA(AQPProjectileBullet::StaticClass()))
	{
		return; 
	}

	const FVector Dir = GetVelocity().GetSafeNormal();
	
	/** 엔진의 표준 PointDamage 시스템을 사용하여 타격 지점 및 방향 정보와 함께 데미지 전달 */
	UGameplayStatics::ApplyPointDamage(OtherActor, Damage, Dir, Hit, GetInstigatorController(), this, DamageTypeClass);
	
	/** 적이나 장애물에 충돌한 즉시 총알을 파괴하여 중복 타격 방지 */
	Destroy(); 
}

void AQPProjectileBullet::Destroyed()
{
	if (TrailFXComponent)
	{
		// 총알이 파괴될 때 남아있는 꼬리(Trail) 이펙트도 즉시 강제로 지워줍니다.
		TrailFXComponent->DestroyComponent(); 
	}

	Super::Destroyed();
}


