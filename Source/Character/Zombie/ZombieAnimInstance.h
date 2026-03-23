// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZombieAnimInstance.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override; //애니메이션 업데이트 재정의
	virtual void NativeInitializeAnimation() override; //애니메이션 초기화 재정의
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Animation")
	TObjectPtr<APawn> OwningPawn; //애니메이션 인스턴스를 소유한 폰
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Animation")
	TObjectPtr<class AZombieCharacter> OwningZombieCharacter; //애니메이션 인스턴스를 소유한 좀비 캐릭터
public:
	//이동속도 (Idle/Walk/Run) 반환 함수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Animation")
	float MovementSpeed = 0.f;
	//공격중(공격 몽타주 상태)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Animation")
	bool bIsAttacking = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
	float SmoothedSpeed = 0.f; //부드러운 이동 속도

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim")
	float SpeedInterpRate = 8.f; // 클수록 빨리 따라감
	UFUNCTION()
	void AnimNotify_AttackHit(); //애님노티파이: 공격 히트
	UFUNCTION()
	void AnimNotify_AttackEnd(); //애님노티파이: 공격 종료
};