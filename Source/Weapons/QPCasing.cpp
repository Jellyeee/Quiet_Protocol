#include "QPCasing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"

AQPCasing::AQPCasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	
	CasingMesh->SetSimulatePhysics(false); // 초기에는 물리 꺼둠
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true); // Hit 이벤트용
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬 충돌 초기 비활성화
	CasingMesh->SetVisibility(false); // 메쉬 초기 숨김

	// 처음 생성될 때는 비활성화 상태
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AQPCasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &AQPCasing::OnHit);
}

void AQPCasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bHasHitGround)
	{
		bHasHitGround = true;
		if (ShellSound)
		{
			float VolumeMultiplier = 1.0f;
			if (UWorld* World = GetWorld())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
					{
						VolumeMultiplier = AudioSubsystem->GetSFXVolume() * AudioSubsystem->GetMasterVolume();
					}
				}
			}

			UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation(), VolumeMultiplier);
		}
	}
}

void AQPCasing::ActivateCasing(const FTransform& EjectTransform, const FVector& WeaponVelocity)
{
	bIsActive = true;
	bHasHitGround = false;

	// 위치 및 회전 설정 (스케일 이슈 방지를 위해 1.0으로 고정)
	FTransform SpawnTransform = EjectTransform;
	SpawnTransform.SetScale3D(FVector(1.f, 1.f, 1.f));
	SetActorTransform(SpawnTransform);
	
	// 액터와 메쉬가 모두 화면에 보이고 물리 연산이 되도록 명시적 활성화
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	CasingMesh->SetVisibility(true);
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CasingMesh->SetSimulatePhysics(true);

	// 탄피 배출 방향 계산 (소켓의 우측/뒤쪽 방향 등으로 설정)
	FVector ForwardDir = EjectTransform.GetRotation().GetForwardVector();
	FVector RightDir = EjectTransform.GetRotation().GetRightVector();
	FVector UpDir = EjectTransform.GetRotation().GetUpVector();

	// 탄피 배출 힘 (총기 우측 상단으로 배출되도록)
	FVector EjectDirection = (RightDir + UpDir * FMath::RandRange(0.5f, 1.0f) - ForwardDir * FMath::RandRange(0.2f, 0.5f)).GetSafeNormal();

	// 총구 움직임 속도 합산
	CasingMesh->SetPhysicsLinearVelocity(WeaponVelocity);
	
	// 배출 방향으로 충격량 적용
	CasingMesh->AddImpulse(EjectDirection * ShellEjectionImpulse * CasingMesh->GetMass());
	
	// 랜덤 회전 적용
	CasingMesh->AddTorqueInRadians(FVector(FMath::RandRange(-0.5f, 0.5f), FMath::RandRange(-0.5f, 0.5f), FMath::RandRange(-0.5f, 0.5f)) * 100.f * CasingMesh->GetMass());

	// 3초 후 비활성화 예약
	GetWorld()->GetTimerManager().SetTimer(DeactivateTimer, this, &AQPCasing::DeactivateCasing, 3.0f, false);
}

void AQPCasing::DeactivateCasing()
{
	bIsActive = false;
	
	// 물리 끄고 숨기기
	CasingMesh->SetSimulatePhysics(false);
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CasingMesh->SetVisibility(false);

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	// 타이머 초기화
	GetWorld()->GetTimerManager().ClearTimer(DeactivateTimer);
}
