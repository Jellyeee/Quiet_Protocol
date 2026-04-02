#include "InventoryRootWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "InventoryDragOperation.h" 

void UInventoryRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	/** 소유 플레이어의 캐릭터로부터 인벤토리 컴포넌트를 가져와 캐싱합니다. */
	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn());
	if (!Character || !InventoryGrid) return;

	CachedInventory = Character->GetInventoryComponent();
	if (!CachedInventory) return;

	/** 인벤토리 데이터 변경 시 UI를 자동으로 갱신하기 위해 델리게이트를 바인딩합니다. (중복 방지를 위해 기존 바인딩 제거 후 수행) */
	CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryRootWidget::HandleInventoryChanged);
	CachedInventory->OnInventoryChanged.AddDynamic(this, &UInventoryRootWidget::HandleInventoryChanged);

	/** 그리드 위젯에 인벤토리 데이터를 전달하고 초기 동기화를 수행합니다. */
	InventoryGrid->SetInventory(CachedInventory);
	InventoryGrid->RefreshGrid();
}

void UInventoryRootWidget::NativeDestruct()
{
	if (CachedInventory)
	{
		CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryRootWidget::HandleInventoryChanged);
		CachedInventory = nullptr;
	}
	Super::NativeDestruct();
}

bool UInventoryRootWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation) return false;
	if (!InventoryGrid) return false;

	/** 드래그 중인 오퍼레이션이 유효한 인벤토리 드래그 작업인지 확인합니다. */
	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(InOperation);
	if (!DragOp) return false;

	/** 드롭된 위치(마우스 좌표)가 인벤토리 그리드 영역 안인지 밖인지 판별합니다. */
	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();
	const FGeometry GridGeo = InventoryGrid->GetCachedGeometry();
	const bool bOverGrid = GridGeo.IsUnderLocation(ScreenPos);

	if (bOverGrid)
	{
		/** 그리드 내부에 드롭된 경우: 아이템 이동 또는 위치 교환 로직을 수행합니다. */
		return InventoryGrid->HandleDropFromScreenPos(InOperation, ScreenPos);
	}

	/** 
	 * 그리드 외부(빈 화면)에 드롭된 경우: 
	 * 현재 드래그 중인 아이템을 캐릭터 위치에 실제로 버리는(Drop to World) 기능을 수행합니다.
	 */
	if (DragOp->SourceInventory) {
		AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn());
		if (!Character) return false;
		
		/** 캐릭터 클래스에 구현된 외부 드랍 함수를 호출합니다. */
		Character->DropInventoryItemAt(DragOp->FromCell);
		return true;
	}
	return false;
}

void UInventoryRootWidget::HandleInventoryChanged()
{
	if (InventoryGrid) InventoryGrid->RefreshGrid();
}
