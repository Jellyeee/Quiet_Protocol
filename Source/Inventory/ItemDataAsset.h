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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mesh", meta = (DisplayName = "Pickup Mesh", ToolTip = "바닥에 떨어졌을 때 보일 일반 아이템 메쉬"))
	class UStaticMesh* PickupMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mesh", meta = (DisplayName = "Pickup Skeletal Mesh", ToolTip = "방어복 등 스켈레탈 메쉬를 사용하는 아이템을 위한 메쉬"))
	class USkeletalMesh* PickupSkeletalMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (DisplayName = "Weapon"))
	TSubclassOf<class AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Ammo", meta = (DisplayName = "Target Weapon Type", ToolTip = "탄약일 경우 어떤 무기에 쓰이는지 설정"))
	EQPWeaponType TargetWeaponType = EQPWeaponType::EWT_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Stats", meta = (DisplayName = "Shield Amount", ToolTip = "방어복 아이템일 경우 추가될 보호막 양"))
	float ShieldAmount = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Stats", meta = (DisplayName = "Heal Amount", ToolTip = "회복 아이템일 경우 추가될 체력 양"))
	float HealAmount = 50.f;
};
