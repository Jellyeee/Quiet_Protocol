#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "QPCombatComponent.generated.h"

class AWeaponBase; //전방 선언
class ACharacter; //전방 선언

/**
무기 타입이 변경될 때 브로드캐스트되는 델리게이트
호출 시점: SetWeaponType() 함수에서 EquippedWeaponType이 실제로 변경될 때
주요 용도:
UI 업데이트 (무기 아이콘, 탄약 표시 등)
애니메이션 블루프린트에서 무기별 애니메이션 전환
사운드/이펙트 시스템에 무기 변경 알림
@param NewWeaponType 새로 장착된 무기의 타입 (EQPWeaponType)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTypeChanged,
	EQPWeaponType, NewWeaponType); //무기 장착 델리게이트 선언
/**
공격 상태가 변경될 때 브로드캐스트되는 델리게이트
호출 시점: 캐릭터가 공격을 시작하거나 멈출 때
주요 용도:
UI 업데이트 (공격 모션 표시 등)
애니메이션 블루프린트에서 공격 애니메이션 전환
사운드/이펙트 시스템에 공격 상태 변경 알림
@param bNewIsAttacking 새로운 공격 상태 (true: 공격 중, false: 비공격 중)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStateChanged, bool, bNewIsAttacking);//공격 상태 변경 델리게이트 선언

/*
 * 조준 상태 변경 델리게이트
 * - AimOffset, 조준 전용 애니메이션, 카메라 전환 등에 사용
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimStateChanged,bool, bNewIsAiming);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PJ_QUIET_PROTOCOL_API UQPCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQPCombatComponent();
	//무기 기능
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	bool EquipWeapon(AWeaponBase* NewWeapon, bool bUnequipCurrent = true); //무기 장착 함수
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	bool UnEquipWeapon(bool bDropToWorld = true); //무기 해제 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	FORCEINLINE AWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; } //장착된 무기 반환 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	FORCEINLINE EQPWeaponType GetEquippedWeaponType() const { return EquippedWeaponType; } //장착된 무기 타입 반환 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	FORCEINLINE bool HasWeapon() const { return EquippedWeapon != nullptr; } //무기 장착 여부 반환 함수

	//공격 기능
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void StartAttack(); //공격 시작 함수
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void StopAttack(); //공격 중지 함수
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Reload")
	void Reload(); // 재장전 함수

	UFUNCTION(BlueprintPure, Category = "Combat|Attack")
	FORCEINLINE bool IsAttacking() const { return bIsAttacking; } //공격 중인지 여부 반환 함수
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Aim") 
	void SetAiming(bool bNewAiming); //조준 상태 설정 함수

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	FORCEINLINE bool IsAiming() const { return bIsAiming; } //조준 중인지 여부 반환 함수

	UFUNCTION(BlueprintPure, Category = "Combat|Wait")
	FVector GetMuzzleHitTarget() const; // 실제 총구가 가리키는 위치 반환
	
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAttackStateChanged OnAttackStateChanged; //공격 상태 변경 델리게이트 인스턴스
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnWeaponTypeChanged OnWeaponTypeChanged; //무기 타입 변경 델리게이트 인스턴스
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAimStateChanged OnAimStateChanged; //조준 상태 변경 델리게이트 인스턴스
	 

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void TraceUnderCrosshairs(FHitResult& TraceHitResult); //조준선 아래 충돌 검사 함수

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming); //조준 상태 서버에 알리는 함수 (네트워크 동기화용)

	UFUNCTION(Server, Reliable)
	void ServerStartAttack(); //공격 시작 서버에 알리는 함수 (네트워크 동기화용)

	UFUNCTION(Server, Reliable)
	void ServerStopAttack(); //공격 중지 서버에 알리는 함수 (네트워크 동기화용)

	UFUNCTION(Server, Reliable)
	void ServerReload(); // 재장전 서버에 알리는 함수 (네트워크 동기화용)

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastReload(); // 재장전 애니메이션/이펙트 동기화용 함수 (모든 클라이언트에서 재장전 효과 재생)

	UFUNCTION(Server, Unreliable)
	void ServerSetHitTarget(const FVector_NetQuantize& TraceHitTarget_Arg); // 서버에 HitTarget을 알리는 함수 (네트워크 최적화용, Unreliable로 설정)

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFire(bool bInIsAiming); // 발사 효과 동기화용 함수 (모든 클라이언트에서 발사 효과 재생, Unreliable로 설정)

public:
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(class AWeaponBase* WeaponToEquip);

	FVector HitTarget; // 실제 총구가 가리키는 위치 (로컬에서 계산, 서버/클라이언트에서 TraceHitTarget과 동기화)

protected:
	UPROPERTY(Replicated)
	FVector TraceHitTarget; // 실제 총구가 가리키는 위치 (서버에서 계산, 클라이언트로 복제)

private:
	bool AttachWeaponToCharacter(AWeaponBase* Weapon); //캐릭터에 무기 부착 함수
	void SetWeaponType(EQPWeaponType NewType); //무기 타입 설정 함수
	void SetIsAttacking(bool bNewIsAttacking); //공격 상태 설정 함수
	void SetIsAiming(bool bNewIsAiming); //조준 상태 설정 함수

	void Fire(); //실제 발사 로직 처리 함수
	void StartFireTimer(); //발사 타이머 시작 함수

	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr; //소유한 캐릭터

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", ReplicatedUsing = OnRep_EquippedWeapon, meta = (AllowPrivateAccess = "true"))
	AWeaponBase* EquippedWeapon = nullptr; //장착된 무기
	
	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (AllowPrivateAccess = "true"))
	EQPWeaponType EquippedWeaponType = EQPWeaponType::EWT_None; //장착된 무기 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (AllowPrivateAccess = "true"))
	FName EquipSocketName = TEXT("WeaponSocket"); //무기 장착 소켓 이름
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack", Replicated, meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false; //공격 중인지 여부
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Aim", Replicated, meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false; // 조준 중인지 여부

	FTimerHandle FireTimer; //발사 간격 제어용 타이머 핸들

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	FVector2D CrosshairScreenOffset; // 화면 중앙으로부터의 오프셋

	FVector LastHitTarget; 
	float LastHitTargetRPCTime = 0.f; // 마지막으로 HitTarget을 서버에 전송한 시간 (네트워크 최적화용)

	double LastFireTime = 0.0; // 마지막 발사 시간 (발사 간격 제어용)
	bool CanFire(bool bAutomatic);  // 발사 가능 여부 체크 함수 (자동/비자동 무기 구분)
	
	bool bCanFireSingleShot = true; // 단발 무기를 누르고 있을 때 연속 발사를 막기 위한 플래그


	// Crosshair Offsets (Hip-Fire)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	FVector2D HipFireCenterOffset = FVector2D(150.f, 120.f); // 정면

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	FVector2D HipFireUpOffset = FVector2D(150.f, 120.f); // 위를 볼 때

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	FVector2D HipFireDownOffset = FVector2D(150.f, 120.f); // 아래를 볼 때

	// Crosshair Offsets (Aiming)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	FVector2D AimingBaseOffset = FVector2D(150.f,  120.f); // 조준 시 기본 위치

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	float AimingVerticalScale = 50.f; // 조준 시 Pitch에 따른 수직 이동 비율

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim|Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrouchCrosshairOffset = 30.f; // 앉아 있을 때 크로스헤어 수직 오프셋 (양수: 아래로, 음수: 위로)

	void UpdateCrosshairPosition(float DeltaTime); // 크로스헤어 위치 업데이트 함수

};
