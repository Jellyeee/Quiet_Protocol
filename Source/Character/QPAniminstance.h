#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "QPAniminstance.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPAniminstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override; // 애니메이션 인스턴스 초기화
	virtual void NativeUpdateAnimation(float DeltaSeconds) override; // 애니메이션 인스턴스 업데이트

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.f; // 이동 속도
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Direction = 0.f; //이동 방향

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false; // 공중에 있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAccelerating = false; // 가속 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsCrouched = false; //앉아있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSprinting = false; // 달리고 있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EQPWeaponType WeaponType = EQPWeaponType::EWT_None; // 장착된 무기 타입
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking = false; // 공격 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsReloading = false; // 장전 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAiming = false; // 조준 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsStopping = false; // 정지 중인지 (관성 이동 중) 여부
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsTurningInPlace = false; // 제자리 회전 중인지 여부

	UPROPERTY(BlueprintReadOnly, Category = "AimOffset") 
	float AO_Yaw; // 애니메이션 오프셋의 Yaw 값
	UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
	float AO_Pitch; // 애니메이션 오프셋의 Pitch 값
	
	FRotator SmoothedControlRotation = FRotator::ZeroRotator; // 부드러운 제어 회전 (조준 시 총구 방향 보정용)
	float SmoothedDeltaYaw = 0.f; // 부드러운 Delta Yaw (제자리 회전 시 자연스러운 회전 보정용)

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FVector RightHandOffset = FVector(0.f, 0.f, 0.f); // 오른손 위치 오프셋 (Y=15: 오른쪽 이동)

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FVector RightElbowOffset; // 오른쪽 팔꿈치 오프셋 (아래 조준 시 거리 확보용)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK")
	FVector ElbowRetractionAmount = FVector(-20.f, 0.f, 0.f); // 아래 조준 시 팔꿈치를 당길 양

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FRotator SpineRotation; // 상체 회전 보정 (아래 조준 시 거리 확보용)

	FVector DefaultRightHandOffset; // 초기 설정값 저장용

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FTransform LeftHandIKTransform; // 왼손 IK 트랜스폼

	UPROPERTY(BlueprintReadOnly, Category = "IK")
	bool bUseLeftHandIK = false; // 왼손 IK 사용 여부

	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	float RootYawOffset = 0.f; // 루트 Yaw 오프셋

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn")
	float MaxTurnAngle = 43.f; // 최대 제자리 회전 각도

	// 총구 방향 보정을 위한 손 회전 값
	UPROPERTY(BlueprintReadOnly, Category = "IK")
	FRotator HandRotationCorrection;

private:
	TWeakObjectPtr <class AQPCharacter> CachedCharacter; // 캐릭터에 대한 약한 포인터
};
