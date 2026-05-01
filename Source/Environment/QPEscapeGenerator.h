#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPEscapeGenerator.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AWorldItemActor;
class UItemDataAsset;

class AWorldItemActor;
class UItemDataAsset;

USTRUCT(BlueprintType)
struct FRewardItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AWorldItemActor> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemDataAsset* ItemData;
};

UCLASS()
class PJ_QUIET_PROTOCOL_API AQPEscapeGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AQPEscapeGenerator();

	// 상호작용 키 유지 시 진행도 증가 (직접 호출 혹은 타이머 스텝용)
	UFUNCTION(BlueprintCallable, Category = "Escape|Generator")
	void AddProgress(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Escape|Generator")
	void StartRepairing(class AQPCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Escape|Generator")
	void StopRepairing(class AQPCharacter* Character);

	// 스킬체크(QTE) 버튼을 눌렀을 때 클라이언트에서 호출
	UFUNCTION(BlueprintCallable, Category = "Escape|Generator")
	void SubmitSkillCheck(class AQPCharacter* Character);

	UFUNCTION(Server, Reliable)
	void ServerStartRepairing(class AQPCharacter* Character);

	UFUNCTION(Server, Reliable)
	void ServerStopRepairing(class AQPCharacter* Character);

	UFUNCTION(Server, Reliable)
	void ServerSubmitSkillCheck(class AQPCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Escape|Generator")
	float GetProgressRatio() const { return MaxProgress > 0.f ? (CurrentProgress / MaxProgress) : 0.f; }

	// 게임 모드에서 호출하여 몇 번째 키카드 번호를 드롭할지 할당
	void AssignCardInfo(int32 SlotIdx, int32 CodeNum);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCompleteRepair();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSkillCheckFailed();

	// 수리 완료 처리
	void CompleteRepair();

	// 아이템 드롭 로직 (오직 서버에서만)
	void DropRewards();

	// 수리 타이머 틱
	void RepairTick();

	void TriggerSkillCheck();
	void FailSkillCheck();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* InteractVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* GeneratorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* ProgressWidgetComp;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentProgress, VisibleAnywhere, BlueprintReadOnly, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	float CurrentProgress;

	UFUNCTION()
	void OnRep_CurrentProgress();

	UPROPERTY(ReplicatedUsing = OnRep_IsBeingRepaired, VisibleAnywhere, BlueprintReadOnly, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	bool bIsBeingRepaired;

	UFUNCTION()
	void OnRep_IsBeingRepaired();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	float MaxProgress = 100.f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	bool bIsCompleted;

	// 할당받은 키카드 정보
	int32 AssignedSlotIndex;
	int32 AssignedCodeNumber;

	FTimerHandle RepairTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category = "Escape|Generator")
	float RepairRatePerSecond = 5.f; // 1초당 차오르는 수리 진행도 (완료에 20초)
	
	UPROPERTY(EditAnywhere, Category = "Escape|Generator|Rewards")
	TArray<FRewardItem> RewardItems;

	UPROPERTY()
	class AQPCharacter* CurrentRepairer;

	// 스킬체크 관련 상태
	bool bIsSkillCheckActive;
	float SkillCheckTimeLeft;
	float SkillCheckTargetStart;
	float SkillCheckTargetEnd;
	float SkillCheckMaxTime;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
