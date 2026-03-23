
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombieCharacter();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

public:
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float WalkSpeed = 150.f; //좀비 걷기 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float ChaseSpeed = 300.f; //좀비 추격 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float AttackPower = 30.f; //좀비 공격력 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float MaxHealth = 100.f; //좀비 최대 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float Health; //좀비 현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	bool bIsDead = false; //좀비 사망 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Sense")
	float DetectRange = 1200.f; //플레이어 감지 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	float AttackRange = 150.f; //좀비 공격 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	float AttackCoolDown = 1.2f; //좀비 공격 쿨타임
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	UAnimMontage* AttackMontage = nullptr; //좀비 공격 모션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	UAnimMontage* DeathMontage = nullptr; //좀비 사망 모션

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Runtime")
	TObjectPtr<AActor> TargetActor = nullptr; //현재 타겟팅된 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Runtime")
	bool bIsAttacking = false; //공격 중인지 여부

	UFUNCTION(BlueprintCallable, Category = "Zombie")
	void SetTarget(AActor* NewTarget); //타겟 설정 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie")
	bool CanAttackTarget() const;//타겟 공격 가능 여부 확인 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie")
	void StartAttack(); //공격 시작 함수

	//AnimNotify에서 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void AttackHit(); //공격 히트 처리 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void AttackEnd(); //공격 종료 처리 함수
	UFUNCTION(BlueprintPure, Category = "Zombie|Combat")
	FORCEINLINE bool IsAttacking() const { return bIsAttacking; } //공격 중인지 여부 반환 함수
	UFUNCTION(BlueprintPure, Category = "Zombie|Combat")
	FORCEINLINE float GetAttackRange() const { return AttackRange; } //공격 범위 반환 함수
private:
	float LastAttackTime = -1000.f; //마지막 공격 시간

	bool bPrevOrientRotationToMovement = true; //이전 회전 방향 플래그
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); //공격 몽타주 종료 콜백 함수

	void EnterAttackRootMotionMode(); //루트 모션 모드 진입 함수
	void ExitAttackRootMotionMode(); //루트 모션 모드 종료 함수

	void Die(); //사망 처리 함수

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie(); //사망 멀티캐스트 처리 함수
};