#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDataAsset.h"
#include "WorldItemActor.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AWorldItemActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AWorldItemActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", Replicated)
	UItemDataAsset* ItemData; //아이템 정보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", Replicated)
	int32 Quantity; //아이템 수량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Password", Replicated)
	int32 AssignedSlotIndex = -1; // -1이면 일반 아이템, 1~4면 키카드 슬롯 번호

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Password", Replicated)
	int32 AssignedCodeNumber = -1; // 해당 슬롯의 비밀번호 숫자

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Pickup")
	TObjectPtr<class USceneComponent> Root; //루트 씬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Pickup")
	TObjectPtr<class USphereComponent> PickupSphere; //픽업 스피어 컴포넌트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Pickup", meta = (ClampMin = "0.0"))
	float PickupSphereRadius = 100.f; //픽업 스피어 반지름

	UFUNCTION()
	void OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<class USkeletalMeshComponent> ItemMesh; //아이템 메쉬 컴포넌트
};
