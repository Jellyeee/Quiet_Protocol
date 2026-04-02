
#include "ZombieCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "AIController.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "BrainComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
//#include "PJ_Quiet_Protocol/Commons/DefineCommons.h"

AZombieCharacter::AZombieCharacter()
{
	PrimaryActorTick.bCanEverTick = true;


	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement();
	MoveComponent->bOrientRotationToMovement = true;
	MoveComponent->RotationRate = FRotator(0.f, 150.f, 0.f);

}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	Health = MaxHealth;
}

void AZombieCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float AZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bIsDead) return 0.f;

	Health -= ActualDamage;
	if (Health <= 0.f)
	{
		Health = 0.f;
		bIsDead = true;
		Die();
	}
	return ActualDamage;
}

void AZombieCharacter::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
	GetCharacterMovement()->MaxWalkSpeed = (TargetActor ? ChaseSpeed : WalkSpeed);
}

bool AZombieCharacter::CanAttackTarget() const
{
	if (!TargetActor) return false;
	if (bIsAttacking) return false;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < AttackCoolDown) return false;
	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return DistanceToTarget <= AttackRange;
}
void AZombieCharacter::StartAttack()
{
	if (!CanAttackTarget()) return; //타겟 공격 불가 시 함수 종료

	UAnimInstance* AnimInstance = (GetMesh() ? GetMesh()->GetAnimInstance() : nullptr); //애니메이션 인스턴스 가져오기

	//DBG_SCREEN(
	//	3001, 1.5f, FColor::Red,
	//	"StartAttack() called. bIsAttacking=%d AnimInst=%s Montage=%s",
	//	bIsAttacking ? 1 : 0,
	//	*GetNameSafe(AnimInstance),
	//	*GetNameSafe(AttackMontage) 
	//);

	if (!AnimInstance || !AttackMontage) {
		bIsAttacking = false;
		ExitAttackRootMotionMode();
		return;
	}
	if (AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		return;
	}
	LastAttackTime = GetWorld()->GetTimeSeconds();
	bIsAttacking = true;
	EnterAttackRootMotionMode();
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AZombieCharacter::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);
	AnimInstance->Montage_Play(AttackMontage);
}

void AZombieCharacter::AttackHit()
{
	if (!TargetActor) return; //타겟이 없으면 함수 종료
	//Root 모션 공격은 위치가 애니로 밀리니깐, 맞는 순간에만 사거리 체크
	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()); //타겟과의 거리 계산
	if (Dist > AttackRange + 30.f) return; //공격 범위 초과 시 함수 종료
	//데미지 구현시에 적용
	UGameplayStatics::ApplyDamage(TargetActor, AttackPower, GetController(), this, UDamageType::StaticClass()); //타겟에 데미지 적용
}

void AZombieCharacter::AttackEnd()
{
	if (!bIsAttacking) return; //공격 중이 아니면 함수 종료
	bIsAttacking = false; //공격 상태 해제
	ExitAttackRootMotionMode(); //루트 모션 모드 종료
	if (UAnimInstance* AnimInstance = (GetMesh() ? GetMesh()->GetAnimInstance() : nullptr))
	{
		if (AttackMontage && AnimInstance->Montage_IsPlaying(AttackMontage))
		{
			AnimInstance->Montage_Stop(0.2f, AttackMontage); //공격 모션 중지
		}
	}
}

void AZombieCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AttackMontage) return;
	bIsAttacking = false;
	ExitAttackRootMotionMode();
}

void AZombieCharacter::EnterAttackRootMotionMode()
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	bPrevOrientRotationToMovement = MoveComponent->bOrientRotationToMovement; //이전 회전 방향 플래그 저장
	MoveComponent->bOrientRotationToMovement = false; //이동 방향으로 회전
	MoveComponent->StopMovementImmediately(); //즉시 이동 중지
	if (AAIController* AIController = Cast<AAIController>(GetController())) //AI 컨트롤러 가져오기
	{
		AIController->StopMovement(); //AI 컨트롤러 이동 중지
	}
}

void AZombieCharacter::ExitAttackRootMotionMode()
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	MoveComponent->bOrientRotationToMovement = bPrevOrientRotationToMovement; //이전 회전 방향 플래그 복원
	MoveComponent->MaxWalkSpeed = (TargetActor ? ChaseSpeed : WalkSpeed); //타겟이 있으면 추격 속도, 없으면 걷기 속도
}

void AZombieCharacter::Die()
{
	MulticastDie();
}

void AZombieCharacter::MulticastDie_Implementation()
{
	// 1. AI 컨트롤러 중지 (비헤이비어 트리 끄기)
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		if (AIController->GetBrainComponent())
		{
			AIController->GetBrainComponent()->StopLogic("Zombie is Dead");
		}
	}

	// 2. 다른 액터와 충돌 방지 (캡슐 콜리전 및 메시 설정)
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	}
	
	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	}

	// 3. 사망 몽타주 재생
	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		float MontageDuration = GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
		
		if (MontageDuration > 0.f)
		{
			TWeakObjectPtr<AZombieCharacter> WeakThis(this);
			FTimerHandle AnimPauseTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(AnimPauseTimerHandle, FTimerDelegate::CreateLambda([WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->GetMesh())
				{
					WeakThis->GetMesh()->bPauseAnims = true; 
				}
			}), FMath::Max(0.1f, MontageDuration - 0.15f), false);
		}
	}
	
	// 4. 루트 모션이나 기타 이동 관련 초기화
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	bIsAttacking = false;
}