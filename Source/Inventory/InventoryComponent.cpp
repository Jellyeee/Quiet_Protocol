#include "InventoryComponent.h"
#include "ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
	if (!ItemData) return false;
	if (Quantity <= 0) return false;

	// 인벤토리의 모든 슬롯을 순회하며 빈 공간 찾기 (왼쪽 상단부터)
	for (int32 x = 0; x < Width; ++x)
	{
		for (int32 y = 0; y < Height; ++y)
		{
			const FIntPoint Position(x, y);
			// 해당 위치에 아이템 추가 시도
			if(AddItemAt(ItemData, Quantity, Position))
			{
				UE_LOG(LogTemp, Warning, TEXT("[INV] AddItem OK: Slots=%d"), Slots.Num());
				return true;
			}
		}
	}
	return false;
}
bool UInventoryComponent::AddItemAt(UItemDataAsset* ItemData, int32 Quantity, const FIntPoint& Position)
{
	if (!ItemData) return false;
	if (Quantity <= 0) return false;
	// 배치 가능 여부 확인
	if (!CanPlaceItemAt(ItemData, Position)) return false;

	// 새 슬롯 생성 및 데이터 설정
	FInventorySlot NewSlot;
	NewSlot.Position = Position;
	NewSlot.Item.ItemData = ItemData;
	NewSlot.Item.Quantity = Quantity;
	
	Slots.Add(NewSlot); // 인벤토리 배열에 추가
	OnInventoryChanged.Broadcast(); // 변경 사항 알림
	return true;
}

bool UInventoryComponent::RemoveItemAt(const FIntPoint& Position)
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Position == Position)
		{
			Slots.RemoveAt(i);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}
	return false;
}

bool UInventoryComponent::MoveItem(const FIntPoint& From, const FIntPoint& To)
{
	if (From == To) return true;

	const int32 FoundIndex = Slots.IndexOfByPredicate([&](const FInventorySlot& S)
		{
			return S.Position == From;
		});

	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	FInventorySlot MovingSlot = Slots[FoundIndex];
	Slots.RemoveAt(FoundIndex);

	if (!CanPlaceItemAt(MovingSlot.Item.ItemData, To))
	{
		Slots.Insert(MovingSlot, FoundIndex);
		return false;
	}

	MovingSlot.Position = To;
	Slots.Add(MovingSlot);
	OnInventoryChanged.Broadcast();
	return true;
}

bool UInventoryComponent::CanPlaceItemAt(UItemDataAsset* ItemData, const FIntPoint& Position)
{
	if (!ItemData) return false;

	// 아이템 크기 보정 (최소 1x1)
	FIntPoint ItemSize = ItemData->ItemSize;
	ItemSize.X = FMath::Max(1, ItemData->ItemSize.X);
	ItemSize.Y = FMath::Max(1, ItemData->ItemSize.Y);

	// 아이템의 모든 칸이 인벤토리 경계 내에 있는지 확인
	for (int32 x = 0; x < ItemSize.X; ++x)
	{
		for (int32 y = 0; y < ItemSize.Y; ++y)
		{
			if (!IsWithinBounds(Position + FIntPoint(x, y)))
			{
				return false;
			}
		}
	}
	// 기존 아이템과의 중첩 여부 확인
	if(IsOverlapping(ItemData, Position))
	{
		return false;
	}
	return true;
}

bool UInventoryComponent::FindSlotAt(const FIntPoint& Position, FInventorySlot& OutSlot) const
{
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Position == Position)
		{
			OutSlot = Slot;
			return true;
		}
	}
	return false;
}

bool UInventoryComponent::FindSlotContaining(const FIntPoint& Cell, FInventorySlot& Outslot) const
{
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.Item.ItemData) continue;

		const FIntPoint SlotPos = Slot.Position;
		FIntPoint SlotSize = Slot.Item.ItemData->ItemSize;
		SlotSize.X = FMath::Max(1, SlotSize.X);
		SlotSize.Y = FMath::Max(1, SlotSize.Y);
		
		const bool bInX = (Cell.X >= SlotPos.X) && (Cell.X < SlotPos.X + SlotSize.X);
		const bool bInY = (Cell.Y >= SlotPos.Y) && (Cell.Y < SlotPos.Y + SlotSize.Y);
		if (bInX && bInY)
		{
			Outslot = Slot;
			return true;
		}
	}
	return false;
}



bool UInventoryComponent::IsWithinBounds(const FIntPoint& Position) const
{
	return Position.X >= 0 && Position.Y >= 0 && Position.X < Width && Position.Y < Height;
}

bool UInventoryComponent::IsOverlapping(UItemDataAsset* ItemData, const FIntPoint& Position) const
{
	if (!ItemData) return false;

	FIntPoint ItemSize = ItemData->ItemSize;
	ItemSize.X = FMath::Max(1, ItemData->ItemSize.X);
	ItemSize.Y = FMath::Max(1, ItemData->ItemSize.Y);

	// 현재 설치된 모든 슬롯과 대조하여 AABB 충돌 검사 수행
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.Item.ItemData) continue;

		const FIntPoint ExistPos = Slot.Position;
		FIntPoint ExistSize = Slot.Item.ItemData->ItemSize;
		ExistSize.X = FMath::Max(1, ExistSize.X);
		ExistSize.Y = FMath::Max(1, ExistSize.Y);

		// X축 및 Y축 중첩 여부 판단
		const bool bOverlapX = Position.X < ExistPos.X + ExistSize.X && Position.X + ItemSize.X > ExistPos.X;
		const bool bOverlapY = Position.Y < ExistPos.Y + ExistSize.Y && Position.Y + ItemSize.Y > ExistPos.Y;
		if(bOverlapX && bOverlapY)
		{
			return true; // 중첩됨
		}
	}
	return false;
}

int32 UInventoryComponent::GetTotalAmmo(EQPWeaponType WeaponType) const
{
	int32 TotalAmt = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item.ItemData && Slot.Item.ItemData->ItemType == EItemType::EIT_Ammo && Slot.Item.ItemData->TargetWeaponType == WeaponType)
		{
			TotalAmt += Slot.Item.Quantity;
		}
	}
	return TotalAmt;
}

int32 UInventoryComponent::ConsumeAmmo(EQPWeaponType WeaponType, int32 AmountToConsume)
{
	int32 Consumed = 0;
	int32 RemainingToConsume = AmountToConsume;

	// 뒤에서부터 순회하여 요소를 제거할 때 인덱스 꼬임을 방지 (LIFO 방식에 가까움)
	for (int32 i = Slots.Num() - 1; i >= 0; --i)
	{
		if (RemainingToConsume <= 0) break;

		FInventorySlot& Slot = Slots[i];
		// 아이템 타입이 탄약이고, 대상 무기 타입이 일치하는지 확인
		if (Slot.Item.ItemData && Slot.Item.ItemData->ItemType == EItemType::EIT_Ammo && Slot.Item.ItemData->TargetWeaponType == WeaponType)
		{
			if (Slot.Item.Quantity <= RemainingToConsume)
			{
				// 슬롯의 탄약을 전부 소모
				Consumed += Slot.Item.Quantity;
				RemainingToConsume -= Slot.Item.Quantity;
				Slots.RemoveAt(i); // 수량이 0이 된 슬롯 제거
			}
			else
			{
				// 슬롯의 탄약 일부만 소모
				Slot.Item.Quantity -= RemainingToConsume;
				Consumed += RemainingToConsume;
				RemainingToConsume = 0;
			}
		}
	}

	// 실제 소모가 일어났다면 인벤토리 변경 알림
	if (Consumed > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	
	return Consumed;
}
