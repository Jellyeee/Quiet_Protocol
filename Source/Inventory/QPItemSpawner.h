#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPItemSpawner.generated.h"

class UDataTable;
class AWorldItemActor;

/**
 * 월드에 배치되어 게임 시작 시 드랍 테이블에 따라 아이템을 스폰하는 액터
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPItemSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AQPItemSpawner();

protected:
	virtual void BeginPlay() override;

	// 아이템 드랍 정보가 포함된 데이터 테이블 (FItemSpawnRow 형식)
	UPROPERTY(EditAnywhere, Category = "Item Spawner")
	TObjectPtr<UDataTable> DropTable;

	// 일반 아이템을 스폰할 때 사용할 기본 클래스
	UPROPERTY(EditAnywhere, Category = "Item Spawner")
	TSubclassOf<AWorldItemActor> WorldItemClass;

	// 스폰될 아이템의 높이 오프셋 (지면 파묻힘 방지)
	UPROPERTY(EditAnywhere, Category = "Item Spawner")
	float SpawnHeightOffset = 30.0f;

private:
	// 드랍 테이블에서 랜덤하게 아이템을 선택하여 스폰하는 로직
	void SpawnRandomItem();
};
