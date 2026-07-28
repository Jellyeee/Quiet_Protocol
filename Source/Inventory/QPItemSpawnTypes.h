#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "QPItemSpawnTypes.generated.h"

class UItemDataAsset;

/**
 * 아이템 드랍 테이블의 한 행을 정의하는 구조체
 */
USTRUCT(BlueprintType)
struct FItemSpawnRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 스폰할 아이템의 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	// 최소 스폰 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	// 최대 스폰 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	// 스폰 확률 가중치 (값이 높을수록 선택될 확률이 높음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// 스폰할 액터 클래스 직접 지정 (지정하지 않으면 무기 데이터의 클래스나 기본 WorldItemActor를 사용합니다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn")
	TSubclassOf<AActor> ActorClassOverride;
};
