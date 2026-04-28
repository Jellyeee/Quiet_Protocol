#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPEscapeDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class PJ_QUIET_PROTOCOL_API AQPEscapeDoor : public AActor
{
	GENERATED_BODY()
	
public:	
	AQPEscapeDoor();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Escape|Door")
	void SubmitPassword(const TArray<int32>& InputPassword);

	virtual void Tick(float DeltaTime) override;

	// 진짜 출구 여부 세팅
	void SetIsRealExit(bool bReal) { bIsRealExit = bReal; }
	bool IsRealExit() const { return bIsRealExit; }

	// 문 열림 효과 (정답 시)
	void OpenDoor();

	// 함정 발동 효과 (정답을 맞췄으나 가짜 출구일 시, 좀비 웨이브)
	void TriggerTrap();

	// 경보 발동 효과 (비밀번호 오답 시, 문열림 없이 광역 좀비 어그로)
	void TriggerWrongPasswordAggro();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOpenDoor();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastTriggerTrap();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastTriggerWrongPasswordAggro();

	UFUNCTION()
	void OnEscapeZoneOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);



private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DoorVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* EscapeZoneVolume;

	UPROPERTY(Replicated, EditAnywhere, Category = "Escape|Door")
	bool bIsRealExit;

	// --- 함정 (Zombie Wave) 관련 ---
	UPROPERTY(EditAnywhere, Category = "Escape|Trap")
	TSubclassOf<class AZombieCharacter> TrapZombieClass;

	UPROPERTY(EditAnywhere, Category = "Escape|Trap")
	int32 TrapZombieCount = 5;

	UPROPERTY(EditAnywhere, Category = "Escape|Trap")
	float TrapZombieSpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Escape|Trap")
	int32 MaxGlobalZombies = 30;

	// 오답 폭주 시 재생할 경고음 (블루프린트에서 사운드 큐 연결)
	UPROPERTY(EditAnywhere, Category = "Escape|Trap")
	class USoundBase* WrongPasswordSound;

	FTimerHandle TrapTimerHandle;
	int32 SpawnedZombieCount = 0;

	UFUNCTION()
	void SpawnTrapZombie();

	// --- 문 이동 관련 ---
	bool bIsOpening;
	FVector StartLocation;
	FVector TargetLocation;
	float CurrentInterpolation;

	UPROPERTY(EditAnywhere, Category = "Escape|Door")
	float OpenHeight = 200.f; // 문이 올라갈 높이

	UPROPERTY(EditAnywhere, Category = "Escape|Door")
	float OpenSpeed = 1.0f; // 문이 열리는 속도 (초당 진행률)

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
