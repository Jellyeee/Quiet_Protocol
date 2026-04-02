#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/InventoryItem.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);//인벤토리 변경 델리게이트(UI갱신 트리거)

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PJ_QUIET_PROTOCOL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(DisplayName="Inventory Width Number", ToolTip="인벤토리 가로 칸 수 입력"))
	int32 Width = 10; //인벤토리 가로 칸 수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (DisplayName = "Inventory Height Number", ToolTip = "인벤토리 세로 칸 수 입력"))
	int32 Height = 6; //인벤토리 세로 칸 수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (DisplayName = "Inventory Slots", ToolTip = "현재 들어간 아이템들"))
	TArray<FInventorySlot> Slots; //현재 들어간 아이템들

	UPROPERTY(BlueprintAssignable, Category = "Inventory") //인벤토리 변경 델리게이트
	FOnInventoryChanged OnInventoryChanged; //인벤토리 변경 델리게이트

	UFUNCTION(BlueprintCallable, Category = "Inventory") //아이템 추가 함수
	bool AddItem(UItemDataAsset* ItemData, int32 Quantity); //아이템 추가 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory") //아이템 제거 함수
	bool RemoveItemAt(const FIntPoint& Position); //특정 위치의 아이템 제거 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory") //아이템 이동 함수
	bool MoveItem(const FIntPoint& From, const FIntPoint& To); //아이템 이동 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory") //해당 위치에 아이템이 배치가 가능한지?
	bool CanPlaceItemAt(UItemDataAsset* ItemData, const FIntPoint& Position); //아이템 배치 가능 여부 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory") // 슬롯 찾기 함수
	bool FindSlotAt(const FIntPoint& Position, FInventorySlot& OutSlot) const; //특정 위치의 슬롯 찾기 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory") //인벤토리 초기화 함수
	bool FindSlotContaining(const FIntPoint& Cell, FInventorySlot& Outslot) const; //특정 셀을 포함하는 슬롯 찾기 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemAt(UItemDataAsset* ItemData, int32 Quantity, const FIntPoint& Position); //특정 위치에 아이템 추가 함수

	UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
	int32 ConsumeAmmo(EQPWeaponType WeaponType, int32 AmountToConsume);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
	int32 GetTotalAmmo(EQPWeaponType WeaponType) const;

private:
	bool IsWithinBounds(const FIntPoint& Position) const; //인벤토리 범위 내에 있는지 확인하는 함수
	bool IsOverlapping(UItemDataAsset* ItemData, const FIntPoint& Position) const; //아이템이 다른 아이템과 겹치는지 확인하는 함수
};
