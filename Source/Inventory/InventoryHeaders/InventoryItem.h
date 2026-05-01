#pragma once

#include "CoreMinimal.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "InventoryItem.generated.h"

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(DisplayName="Item Data", ToolTip="아이템 데이터 참조값"))
	UItemDataAsset* ItemData = nullptr; //실제 아이템 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (DisplayName = "Quantity", ToolTip = "아이템 수량"))
	int Quantity = 1; //기본 1개

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Keycard")
	int32 AssignedSlotIndex = -1; // -1이면 일반 아이템, 1~4면 키카드 슬롯 번호

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Keycard")
	int32 AssignedCodeNumber = -1; // 해당 키카드의 비밀번호 숫자
};

// FInventoryItem 구조체는 인벤토리 시스템에서 사용되는 아이템 정보를 담고 있다.

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(DisplayName="Grid Position", ToolTip="인벤토리 그리드 좌표"))
	FIntPoint Position; //인벤토리 그리드 좌표

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (DisplayName = "Item", ToolTip = "인벤토리 아이템 정보"))
	FInventoryItem Item; //인벤토리 아이템 정보
};
