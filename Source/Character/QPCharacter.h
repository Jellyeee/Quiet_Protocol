#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "QPCharacter.generated.h"

class UQPStatusComponent; // 전방 선언
class UQPCombatComponent; // 전방 선언
class AWeaponBase; // 전방 선언
class UInventoryComponent; // 전방 선언

/**
 * AQPCharacter
 * 
 * 플레이어 캐릭터의 기본 클래스로, 이동, 전투, 인벤토리, 카메라 제어 및 네트워크 동기화를 담당합니다.
 * 주요 기능:
 * - 이동 시스템 (걷기, 달리기, 앉기)
 * - 전투 시스템 (컴포넌트 기반 무기 장착 및 공격)
 * - 인벤토리 시스템 (아이템 획득, 버리기, 장착)
 * - 카메라 동적 제어 (조준, 앉기, 달리기 시 FOV 변화)
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AQPCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	FORCEINLINE AWeaponBase* GetOverlappingWeapon() const { return OverlappingWeapon; }
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE FVector GetDesiredCameraOffset() const 
	{ 
		bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
		if (IsAiming() && bIsHoldingGun)
		{
			FVector Result = AimingCameraOffset;
			// 조준 시에는 앉기 여부와 상관없이 AimingCameraOffset을 기본으로 사용하지만, 앉은 상태에서는 높이 보정을 추가로 적용
			if (bIsCrouched) 
			{
				// 서있을 때와 앉았을 때의 높이 차이만큼 조준 시 높이도 낮춤
				float HeightDiff = StandingCameraOffset.Z - CrouchCameraPosOffset.Z;
				Result.Z -= HeightDiff;
			}
			return Result;
		}
		return bIsCrouched ? CrouchCameraPosOffset : StandingCameraOffset; 
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE UQPCombatComponent* GetCombatComponent() const { return CombatComponent; }
	UFUNCTION(BlueprintPure, Category = "Status")
	FORCEINLINE UQPStatusComponent* GetStatusComponent() const { return StatusComponent; }
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE EQPWeaponType GetWeaponType() const { return Weapontype; }
	/** 캐릭터가 현재 달리기 상태인지 확인 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsSprinting() const;

	/** 캐릭터가 현재 조준 상태인지 확인 */
	bool IsAiming() const; 

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE class UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; } //인벤토리 컴포넌트 반환 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory|Pickup")
	void SetOverlappingWorldItem(class AWorldItemActor* WorldItem); //겹쳐진 월드 아이템 설정 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory|Pickup")
	class AWorldItemActor* GetOverlappingWorldItem() const { return OverlappingWorldItem; } //겹쳐진 월드 아이템 반환 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context")
	void EquipInventoryItemAt(const FIntPoint& Cell); //인벤토리 아이템 장착 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context")
	void DropInventoryItemAt(const FIntPoint& Cell); //인벤토리 아이템 버리기 함수

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw;  } // 현재 애니메이션 오프셋의 Yaw 값을 반환
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch;  } // 현재 애니메이션 오프셋의 Pitch 값을 반환
	FORCEINLINE bool IsTurningInPlace() const { return bIsTurningInPlace; } // 제자리 회전 중인지 반환

	/** 무기 발사 애니메이션 재생 (조준 여부에 따라 다른 몽타주 재생 지원) */
	void PlayFireMontage(bool bAming); 

	/** 재장전 애니메이션 재생 */
	void PlayReloadMontage(); 

	/** 데미지 처리 (표준 TakeDamage 오버라이드) */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 캐릭터 사망 여부 반환 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const; 

	/** 달리기 강제 중지 */
	void StopSprint(); 

	/** 현재 상태(조준, 앉기 등)에 맞춰 걷기 속도 갱신 */
	void UpdateMovementSpeed(); 

	/** 사망 로직 실행 (래그돌 전환, 카메라 상태 변경 등) */
	void Die(); 

protected:
	virtual void BeginPlay() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override; //앉기 시작시 호출
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override; //일어서기 시작시 호출


	// =============================================================================
	// 컴포넌트 및 기본 설정
	// =============================================================================

	/** 전투 로직(무기 관리, 공격 등)을 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UQPCombatComponent* CombatComponent; 

	/** 캐릭터 상태(체력, 스태미나 등)를 관리하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	UQPStatusComponent* StatusComponent; 

	/** 3인칭 카메라 거리 조절을 위한 스프링 암 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom; 

	/** 플레이어 시점을 담당하는 카메라 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera; 

	/** 현재 장착 중인 무기의 종류 (애니메이션 및 속도 결정에 사용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	EQPWeaponType Weapontype = EQPWeaponType::EWT_None; 

	/** 무기 타입이 변경되었을 때 호출되는 이벤트 핸들러 */
	UFUNCTION()
	void HandleWeaponTypeChanged(EQPWeaponType NewWeaponType); 
	
	/** 조준 상태 변경 시 이동 속도 등을 동기화하는 핸들러 */
	UFUNCTION()
	void HandleAimStateChanged(bool bIsAiming); 

	//움직임 속도 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.f; //걷기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f; //달리기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 300.f; //앉기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSprintSpeed = 700.f; //앉은 상태에서 달리기 속도

	FRotator StartingAimRotation; //시작 에임 회전 값

	//앉기 카메라 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	FVector StandingCameraOffset = FVector(0.f, 0.f, 120.f); 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	FVector CrouchCameraPosOffset = FVector(0.f, 0.f, 140.f); 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float CrouchCameraInterpSpeed = 12.f; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraArmLength = 300.f; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AimingArmLength = 200.f; 
	float DefaultArmLength;

	// ======================================
	// 달리기(Sprint) 이펙트 변수들
	// ======================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sprint")
	TSubclassOf<class UCameraShakeBase> SprintCameraShakeClass; // 달리기 카메라 흔들림 클래스

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sprint")
	float SprintFOV = 105.f;  // 달리기 시 FOV 값 (기본값보다 넓게 설정하여 속도감 증가 효과)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sprint")
	float DefaultFOV = 90.f;  // 기본 FOV 값 (달리기 시작 시와 멈출 때 원래대로 돌아가기 위한 값)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sprint")
	float SprintFOVInterpSpeed = 10.f; // 달리기 시 FOV 보간 속도 (값이 클수록 빠르게 변화)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float MinVerticalArmLength = 250.f; // 앉기 시 카메라가 너무 가까워지는 것을 방지하기 위한 최소 스프링암 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float MaxVerticalArmLength = 600.f; // 앉기 시 카메라가 너무 멀어지는 것을 방지하기 위한 최대 스프링암 길이

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim")
	FVector AimingCameraOffset = FVector(0.f, 40.f, 80.f);  // 조준 시 카메라 오프셋 (앞으로, 옆으로, 위로)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (ClampMin = "0.0"))
	float EquipTraceDistance = 250.f; //무기 장착 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
	bool bDrawEquipTraceDebug = false; //무기 장착 거리 디버그 선 그리기 여부

	//입력 함수들
	void MoveForward(float Value); //앞뒤 이동
	void MoveRight(float Value); //좌우 이동
	void Turn(float Value); //좌우 회전
	void LookUp(float Value); //상하 회전
	void StartJump(); //점프 시작
	void StopJump(); //점프 멈춤
	void ToggleCrouch(); //앉기/일어서기 토글
	void StartSprint(); //달리기 시작

	void EquipPressed(); //장착 버튼 눌림
	void EquipReleased(); //장착 해제 버튼 눌림
	void OnEquipHoldTriggered(); //장착 홀드 트리거
	void TryEquipWeapon(); //짧게 누를때 장착	
	void TryStorePickupToInventory(); //아이템을 인벤토리에 저장 시도

	void AttackPressed(); //공격 버튼 눌림
	void AttackReleased(); //공격 버튼 떼짐

	void DropPressed(); //버리기 버튼 눌림
	void DropReleased(); //버리기 버튼 떼짐
	void OnDropHoldTriggered(); //버리기 홀드 트리거
	void TryDropEquipped(); //실제 드랍 실행 로직 함수

	void AimButtonPressed(); //조준 버튼 눌림
	void AimButtonReleased(); //조준 버튼 떼짐
	void ReloadButtonPressed(); //재장전 버튼 눌림
	void AimOffset(float DeltaTime); //에임오프셋 계산
private:
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage; // 재장전 몽타주

	// =============================================================================
	// [Network] 서버 통신 함수 (RPC)
	// =============================================================================

	/** 클라이언트에서 달리기를 시작할 때 서버에 알림 */
	UFUNCTION(Server, Reliable)
	void ServerStartSprint();

	/** 클라이언트에서 달리기를 멈출 때 서버에 알림 */
	UFUNCTION(Server, Reliable)
	void ServerStopSprint();

	/** 상호작용한 무기를 서버에서 장착 처리 */
	UFUNCTION(Server, Reliable)
	void ServerEquipOverlappingWeapon(class AWeaponBase* Weapon);

	/** 월드에 존재하던 아이템 액터를 서버에서 제거 (인벤토리 획득 시) */
	UFUNCTION(Server, Reliable)
	void ServerDestroyPickupActor(class AActor* PickupActor);

	/** 특정 클래스의 무기를 서버에서 스폰하고 장착 */
	UFUNCTION(Server, Reliable)
	void ServerSpawnAndEquipWeapon(TSubclassOf<class AWeaponBase> WeaponClass);

	/** 버려진 아이템을 월드 좌표에 서버에서 생성 */
	UFUNCTION(Server, Reliable)
	void ServerSpawnWorldItem(class UItemDataAsset* ItemData, int32 Quantity, FVector Location);

	/** 장착 중인 무기를 버리는 로직을 서버에서 실행 */
	UFUNCTION(Server, Reliable)
	void ServerTryDropEquipped();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void UpdateCameraDynamics(float DeltaTime);
	void UpdateDeathCamera(float DeltaTime);
	void UpdateRotationMode();
	
	// 소음(발소리) 발생 관련 로직
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UPawnNoiseEmitterComponent* NoiseEmitterComponent;
	
	float FootstepNoiseTimer = 0.f;
	void UpdateFootstepNoise(float DeltaTime);
	
	// 사망 시 관전 카메라 이동 처리
	void HandleDeathCameraInput(FVector MoveDirection, float Value);

	void UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트 함수
	void RefreshPickupCandidate(const AActor* ActorToIgnore = nullptr); //발밑에 남아있는 픽업 대상을 재탐색해서 타겟 복구

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting) // [Network] Sprint 상태 동기화 (OnRep 추가)
	bool bWantsToSprint = false; //달리기 의사 여부
	
	UFUNCTION()
	void OnRep_IsSprinting(); // [Network] Sprint 상태 변경 시 호출되는 함수

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (AllowPrivateAccess = "true"))
	AWeaponBase* OverlappingWeapon = nullptr; //장착된 무기 포인터

	FVector2D MoveInputVector = FVector2D::ZeroVector; //현재 이동 입력 상태 Sprint 가능 여부 판단용 (앞으로 갈때만 Sprint 가능)
	
	UPROPERTY(Replicated) // [Network] Turn In Place 상태 동기화
	bool bIsTurningInPlace = false; //제자리 회전 중인지 여부
	UPROPERTY(Replicated) // [Network] Aim Offset Yaw 동기화
	float AO_Yaw; //애니메이션 오프셋 Yaw 값
	float AO_Pitch; //애니메이션 오프셋 Pitch 값

	UPROPERTY(Replicated) // [Network] 절대 조준 Yaw 값 (For Stable IK)
	float NetAimYaw; 

	UPROPERTY()
	class AWorldItemActor* OverlappingWorldItem = nullptr; //겹쳐진 월드 아이템 액터

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInventoryComponent> InventoryComponent; //인벤토리 컴포넌트

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	float EquipHoldThreshhold = 0.30f; //E를 이 시간 이상 누르면 인벤토리 저장
	FTimerHandle EquipHoldTimerHandle; //장착 홀드 타이머 핸들
	bool bEquipKeyDown = false; //장착 키가 눌려있는지 여부
	bool bEquipHoldConsumed = false; //홀드로 이미 처리 했는지 확인 불리언 값

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	float DropHoldThreshhold = 0.30f; //G를 이 시간 이상 누르면 드랍
	
	FTimerHandle DropHoldTimerHandle; //드랍 홀드 타이머 핸들
	bool bDropKeyDown = false; //드랍 키가 눌려있는지 여부
	bool bDropHoldConsumed = false; //홀드로 이미 처리 했는지 확인 불리언 값

public:
	FORCEINLINE float GetNetAimYaw() const { return NetAimYaw; }

	int32 MeleeAttackIndex = 0; // 근접 공격 콤보 인덱스

	// 사망 후 카메라 전환
	bool bIsDeathCameraTransitioning = false;
	float DeathCameraTargetArmLength = 1000.f; // 사망 시 카메라가 목표로 하는 스프링암 길이
	FRotator DeathCameraTargetRotation = FRotator(-90.f, 0.f, 0.f); // 목표 회전각 (위에서 아래로)
	
	// 목표 위치 도달 확인 후 자유시점 활성화를 위함
	bool bIsDeathCameraFreeMode = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	UAnimMontage* DeathMontage; // 사망 몽타주

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie(); // 사망 처리 전송 함수

protected:
};
