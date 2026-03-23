#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class ACharacter;

UCLASS()
class PJ_QUIET_PROTOCOL_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE EQPWeaponType GetWeaponType() const { return WeaponType; } //무기 타입 반환 함수
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; } //무기 메쉬 반환 함수

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE float GetFireRate() const { return FireRate; } //발사 간격 반환 함수
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; } //연사 가능 여부 반환 함수

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnEquipped(ACharacter* NewOwner); //무기 장착 시 호출 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnUnequipped(bool bDropToWorld); //무기 해제 시 호출 함수

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void StartFire(); //발사 시작 함수
	virtual void StartFire_Implementation(); //기본 구현
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void StopAttack(); //공격 중지 함수
	virtual void StopAttack_Implementation(); //기본 구현
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Pickup")
	TObjectPtr<class USphereComponent> PickupSphere; //무기 픽업용 스피어 컴포넌트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Pickup", meta = (ClampMin = "0.0"))
	float PickupSphereRadius = 120.f; //픽업 스피어 반지름
	UFUNCTION()
	void OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult); //픽업 시작 오버랩 처리 함수
	UFUNCTION()
	void OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex); //픽업 종료 오버랩 처리 함수

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh; //무기 메쉬 컴포넌트 // TObjectPtr - 자동 null 처리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EQPWeaponType WeaponType = EQPWeaponType::EWT_None; //무기 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 10.f; //기본 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	TSubclassOf<UDamageType> DamageTypeClass; //데미지 타입 클래스

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Fire")
	float FireRate = 0.15f; //발사 간격 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Fire")
	bool bAutomatic = true; //연사 여부

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	class UAnimMontage* FireMontage; //발사 몽타주
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	class UAnimMontage* ReloadMontage; //재장전 몽타주

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Inventory", meta = (DisplayName = "Weapon Item Data (Inventory)", ToolTip = "E를 홀드하면 인벤토리에 저장할 때 사용할 아이템데이터 에셋"))
	TObjectPtr<class UItemDataAsset> WeaponItemData = nullptr; //무기 아이템 데이터 에셋 포인터

public:
	FORCEINLINE UAnimMontage* GetFireMontage() const { return FireMontage; } //발사 몽타주 반환
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; } //재장전 몽타주 반환

	UFUNCTION(BlueprintPure, Category = "Weapon|Inventory")
	FORCEINLINE class UItemDataAsset* GetWeaponItemData() const { return WeaponItemData; } //무기 아이템 데이터 에셋 반환 함수



};