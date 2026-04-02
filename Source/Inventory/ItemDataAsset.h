#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/ItemTypes.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "ItemDataAsset.generated.h"


UCLASS(BlueprintType)
class PJ_QUIET_PROTOCOL_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Name", ToolTip="아이템 이름 기입"))
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Description", ToolTip = "아이템에 대한 설명 기입"))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Icon", ToolTip = "아이템 아이콘 설정"))
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Type", ToolTip = "아이템 타입 설정"))
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Grid Size", ToolTip = "인벤토리 그리드 크기 조정"))
	FIntPoint ItemSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (DisplayName = "Weapon"))
	TSubclassOf<class AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Ammo", meta = (DisplayName = "Target Weapon Type", ToolTip = "탄약일 경우 어떤 무기에 쓰이는지 설정"))
	EQPWeaponType TargetWeaponType = EQPWeaponType::EWT_None;
};
