#include "ZombieAnimInstance.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "PJ_Quiet_Protocol/Character/Zombie/ZombieCharacter.h"
//#include "PJ_Quiet_Protocol/Commons/DefineCommons.h"

void UZombieAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningPawn = TryGetPawnOwner(); //애니메이션 인스턴스를 소유한 폰 가져오기
	OwningZombieCharacter = Cast<AZombieCharacter>(OwningPawn); //좀비 캐릭터로 캐스팅
}

void UZombieAnimInstance::AnimNotify_AttackHit()
{
	if (OwningZombieCharacter) //좀비 캐릭터가 유효한 경우
	{
		//DBG_SCREEN(4001, 1.0f, FColor::Orange, "AnimNotify: AttackHit");
		OwningZombieCharacter->AttackHit(); //좀비 캐릭터의 공격 히트 처리 함수 호출
	}
}

void UZombieAnimInstance::AnimNotify_AttackEnd()
{
	if (OwningZombieCharacter) //좀비 캐릭터가 유효한 경우
	{
		//DBG_SCREEN(4002, 1.0f, FColor::Orange, "AnimNotify: AttackEnd");
		OwningZombieCharacter->AttackEnd(); //좀비 캐릭터의 공격 종료 처리 함수 호출
	}
}

void UZombieAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningPawn) {
		OwningPawn = TryGetPawnOwner(); //애니메이션 인스턴스를 소유한 폰 가져오기
	}
	if (!OwningZombieCharacter && OwningPawn) {
		OwningZombieCharacter = Cast<AZombieCharacter>(OwningPawn); //좀비 캐릭터로 캐스팅
	}
	if (!OwningPawn) return; //소유한 폰이 없으면 반환

	const FVector Velocity = OwningPawn->GetVelocity(); //폰의 속도 가져오기
	const float RawSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size(); //수평 속도 크기 계산
	SmoothedSpeed = FMath::FInterpTo(SmoothedSpeed, RawSpeed, DeltaSeconds, SpeedInterpRate); //부드러운 속도 계산
	MovementSpeed = SmoothedSpeed; //이동 속도 업데이트
	if (OwningZombieCharacter) {
		bIsAttacking = OwningZombieCharacter->bIsAttacking; //좀비 캐릭터의 공격 상태 가져오기
	}
}

