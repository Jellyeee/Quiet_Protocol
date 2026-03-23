// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "TimerManager.h"
#include "GunWeapon.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AGunWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AGunWeapon(); //생성자

	virtual void StartFire_Implementation() override; //발사 시작 함수 재정의
	virtual void StopAttack_Implementation() override; //공격 중지 함수 재정의

protected:
	virtual void BeginPlay() override; // 샷건/권총 초기화 설정용
	
	void FireOnce(); //한 번 발사 함수
	void FireSinglePellet(); // 단일 투사체 발사 기능 (샷건 순차 발사 지원)

	FTimerHandle ShotgunFireTimerHandle; // 샷건 발사 간격 타이머 핸들
	int32 PelletsFiredCount = 0; // 지금까지 발사된 샷건 총알 개수 추적용

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun", meta = (ClampMin = "0.0"))
	float Range = 15000.f; //사거리

	// Projectile
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	TSubclassOf<class AQPProjectileBullet> ProjectileBulletClass; //투사체 불릿 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	float BulletSpeed = 15000.f; //불릿 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	FName MuzzleSocketName = TEXT("MuzzleSocket"); //총구 소켓 이름
	
	// Shotgun Specific
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "1"))
	int32 ShotgunPelletCount = 8; // 샷건 발사 시 총알 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "0.0"))
	float ShotgunSpreadAngle = 5.f; // 샷건 탄착군 퍼짐 각도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "0.01"))
	float TimeBetweenPellets = 0.02f; // 샷건 총알 1발당 발사 간격

	// Handgun Specific
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Handgun", meta = (ClampMin = "0.0"))
	float HandgunRange = 5000.f; // 권총 사거리 (소총보다 짧게)

};
