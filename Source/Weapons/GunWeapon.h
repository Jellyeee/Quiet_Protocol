#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "TimerManager.h"
#include "GunWeapon.generated.h"

/**
 *
 */
/**
 * AGunWeapon
 * 
 * 원거리 공격이 가능한 총기류 무기의 기본 클래스입니다.
 * 탄약 관리, 발사 로직(단발/연사), 반동(Recoil), 그리고 투사체(Projectile) 스폰을 담당합니다.
 * AWeaponBase를 상속받아 장착 및 기본 공격 인터페이스를 구현합니다.
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AGunWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AGunWeapon();

	virtual void StartFire_Implementation() override;
	virtual void StopAttack_Implementation() override;

	/** 현재 남은 장탄수 반환 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Gun|Ammo")
	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/** 최대 장탄수(탄창 용량) 반환 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Gun|Ammo")
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	/** 탄약 추가 (아이템 획득 또는 재장전 시 사용) */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Gun|Ammo")
	void AddAmmo(int32 AmountToAdd);

	/** 탄환 1발 소비 */
	void SpendRound();

	// Recoil Getters
	float GetRecoilPitchMin() const { return RecoilPitchMin; }
	float GetRecoilPitchMax() const { return RecoilPitchMax; }
	float GetRecoilYawMin() const { return RecoilYawMin; }
	float GetRecoilYawMax() const { return RecoilYawMax; }

protected:
	virtual void BeginPlay() override; // 샷건/권총 초기화 설정용
	
	void FireOnce();
	void FireSinglePellet(); // 단일 투사체 발사 기능 (샷건 순차 발사 지원)

	FTimerHandle ShotgunFireTimerHandle; // 샷건 발사 간격 타이머 핸들
	int32 PelletsFiredCount = 0; // 지금까지 발사된 샷건 총알 개수 추적용

	/** 무기 사거리 (표준 사거리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun", meta = (ClampMin = "0.0"))
	float Range = 15000.f; 

	/** [Network] 현재 장탄수 (서버에서 클라이언트로 복제됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon|Gun|Ammo")
	int32 CurrentAmmo = 0;

	/** 최대 장탄수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Gun|Ammo", meta = (ClampMin = "1"))
	int32 MagCapacity = 30; 

	// ======================================
	// 투사체 (Projectile) 설정
	// ======================================
	
	/** 발사 시 스폰될 투사체 클래스 (AQPProjectileBullet 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	TSubclassOf<class AQPProjectileBullet> ProjectileBulletClass; 

	/** 투사체의 초기 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	float BulletSpeed = 15000.f; 

	/** 총구 위치를 결정하는 소켓 이름 (메시 내 소켓 명칭과 일치해야 함) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gun|Projectile")
	FName MuzzleSocketName = TEXT("MuzzleSocket"); 
	
	// ======================================
	// 샷건(Shotgun) 전용 설정
	// ======================================

	/** 샷건 발사 시 한 번에 나가는 산탄(Pellet)의 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "1"))
	int32 ShotgunPelletCount = 8; 

	/** 산탄이 퍼지는 최대 각도 (원뿔 형태) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "0.0"))
	float ShotgunSpreadAngle = 5.f; 

	/** [사용되지 않음] 샷건 총알 간 발사 간격 (현재는 동시 발사 로직 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Shotgun", meta = (ClampMin = "0.01"))
	float TimeBetweenPellets = 0.02f; 

	// ======================================
	// 권총(Handgun) 전용 설정
	// ======================================

	/** 권총 전용 짧은 사거리 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Handgun", meta = (ClampMin = "0.0"))
	float HandgunRange = 5000.f; 

	// ======================================
	// 반동 (Recoil) 설정
	// ======================================

	/** 발사 시 카메라가 위로 튀는 최소 각도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Recoil", meta = (ClampMin = "0.0"))
	float RecoilPitchMin = 0.5f; 

	/** 발사 시 카메라가 위로 튀는 최대 각도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Recoil", meta = (ClampMin = "0.0"))
	float RecoilPitchMax = 1.0f; 

	/** 발사 시 카메라가 좌우로 흔들리는 최소 범위 (음수: 왼쪽) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Recoil")
	float RecoilYawMin = -0.3f; 

	/** 발사 시 카메라가 좌우로 흔들리는 최대 범위 (양수: 오른쪽) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Gun|Recoil")
	float RecoilYawMax = 0.3f; 

};
