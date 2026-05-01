#include "InventoryGridWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/InventoryItem.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "ItemIconWidget.h"
#include "InventoryDragOperation.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "PJ_Quiet_Protocol/UserWidget/Inventory/ItemDragVisualWidget.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모 클래스의 NativeConstruct 호출
}

void UInventoryGridWidget::SetInventory(UInventoryComponent* InInventory)
{
	Inventory = InInventory; // 인벤토리 컴포넌트 설정
}

void UInventoryGridWidget::RefreshGrid()
{
	if (!Inventory || !CellsLayer || !ItemsLayer) return; // 인벤토리와 레이어가 유효한지 확인
	if (GridSizeBox) // GridSizeBox가 유효한지 확인
	{
		GridSizeBox->SetWidthOverride(Inventory->Width * CellSize); // 가로 크기 설정
		GridSizeBox->SetHeightOverride(Inventory->Height * CellSize); // 세로 크기 설정
	} 
	CellsLayer->ClearChildren(); // 셀 레이어 초기화
	ItemsLayer->ClearChildren(); // 아이템 레이어 초기화

	BuildCells(); // 셀 구성
	BuildItems(); // 아이템 구성

	UpdateOccluedCells(); // 가려진 셀 업데이트(0130 추가)
}

void UInventoryGridWidget::BuildCells()
{
	if (!Inventory || !CellsLayer || !CellWidgetClass) return; // 인벤토리와 레이어, 셀 위젯 클래스가 유효한지 확인

	CellWidgets.Reset(); // 셀 위젯 배열 초기화(0130 추가)
	CellWidgets.SetNum(Inventory->Width * Inventory->Height); // 셀 위젯 배열 크기 설정(0130 추가)

	for (int32 y = 0; y < Inventory->Height; ++y) // 세로 셀 반복
	{
		for (int32 x = 0; x < Inventory->Width; ++x) // 가로 셀 반복
		{
			UUserWidget* Cell = CreateWidget<UUserWidget>(GetOwningPlayer(), CellWidgetClass); // 셀 위젯 생성
			if (!Cell) continue; // 생성 실패 시 다음으로

			UCanvasPanelSlot* CellCanvasSlot = CellsLayer->AddChildToCanvas(Cell); // 셀 레이어에 추가
			if (!CellCanvasSlot) continue; // 추가 실패 시 다음으로

			CellCanvasSlot->SetAutoSize(false); // 자동 크기 조정 비활성화
			CellCanvasSlot->SetPosition(FVector2D(x * CellSize, y * CellSize)); // 위치 설정
			CellCanvasSlot->SetSize(FVector2D(CellSize, CellSize)); // 크기 설정

			CellWidgets[CellIndex(x, y)] = Cell; // 셀 위젯 배열에 저장(0130 추가)
		}
	}
}

void UInventoryGridWidget::BuildItems() // 아이템 구성
{
	if (!Inventory || !ItemsLayer || !ItemIconWidgetClass) return; // 인벤토리와 레이어, 아이템 아이콘 위젯 클래스가 유효한지 확인

	for (const FInventorySlot& InvSlot : Inventory->Slots) // 인벤토리 슬롯 반복
	{
		if (!InvSlot.Item.ItemData) continue; // 아이템 데이터가 유효하지 않으면 다음으로

		UItemDataAsset* ItemData = InvSlot.Item.ItemData; // 아이템 데이터 가져오기

		FIntPoint Size = ItemData->ItemSize; // 아이템 크기 가져오기
		Size.X = FMath::Max(1, Size.X); // 최소 크기 1로 설정
		Size.Y = FMath::Max(1, Size.Y); // 최소 크기 1로 설정

		UItemIconWidget* Icon = CreateWidget<UItemIconWidget>(GetOwningPlayer(), ItemIconWidgetClass); // 아이템 아이콘 위젯 생성
		if (!Icon) continue; // 생성 실패 시 다음으로

		Icon->Setup(Inventory, ItemData, InvSlot.Item.Quantity, InvSlot.Position, Size, CellSize, DragVisualClass, this); // 아이템 아이콘 설정

		UCanvasPanelSlot* CanvasSlot = ItemsLayer->AddChildToCanvas(Icon); // 아이템 레이어에 추가
		if (!CanvasSlot) continue; // 추가 실패 시 다음으로

		CanvasSlot->SetAutoSize(false); // 자동 크기 조정 비활성화
		CanvasSlot->SetPosition(FVector2D(InvSlot.Position.X * CellSize, InvSlot.Position.Y * CellSize)); // 위치 설정
		CanvasSlot->SetSize(FVector2D(Size.X * CellSize, Size.Y * CellSize)); // 크기 설정
	}
}

bool UInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	return HandleDropFromScreenPos(InOperation, InDragDropEvent.GetScreenSpacePosition()); // 화면 좌표에서 드롭 처리
}

bool UInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const bool bSuper = Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation); // 부모 클래스의 드래그 오버 처리

	if (!Inventory || !CellsLayer || !InOperation) return bSuper; // 인벤토리와 레이어, 드래그 오퍼레이션이 유효한지 확인

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(InOperation); // 드래그 오퍼레이션 캐스트
	if (!DragOp || !DragOp->ItemData) return bSuper; // 유효성 검사

	// 드래그 비주얼 얻기
	UItemDragVisualWidget* DragVisual = Cast<UItemDragVisualWidget>(DragOp->DefaultDragVisual); // 드래그 비주얼 캐스트

	// 마우스 기준 아이템 좌상단 스크린 좌표
	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition(); // 마우스 스크린 좌표
	const FVector2D TopLeftScreen = ScreenPos - DragOp->DragLocalOffset; // 아이템 좌상단 스크린 좌표 계산

	// 그리드 로컬 -> 셀 좌표
	FIntPoint ToCell(0, 0); // 드롭할 셀 좌표 초기화
	{
		const FGeometry Geo = CellsLayer->GetCachedGeometry(); // 셀 레이어의 기하학 정보 얻기
		const FVector2D Local = Geo.AbsoluteToLocal(TopLeftScreen); // 로컬 좌표로 변환
		ToCell.X = FMath::FloorToInt(Local.X / CellSize); // 셀 X 좌표 계산
		ToCell.Y = FMath::FloorToInt(Local.Y / CellSize); // 셀 Y 좌표 계산
	}

	const bool bCanPlace = CanPlaceForDragPreview(DragOp, ToCell); // 드래그 미리보기용 배치 가능 여부 확인

	// 배치 가능하면 테두리 가능 색으로 표시
	if (DragVisual)
	{
		DragVisual->SetPlacementState(bCanPlace); // 드래그 비주얼에 배치 상태 설정
	}

	return true; 
}

void UInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation); // 부모 클래스의 드래그 리브 처리

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(InOperation); // 드래그 오퍼레이션 캐스트
	if (!DragOp) return; // 유효성 검사

	if (UItemDragVisualWidget* DragVisual = Cast<UItemDragVisualWidget>(DragOp->DefaultDragVisual)) // 드래그 비주얼 캐스트
	{
		//그리드 밖으로 나가면 숨기거나 불가 상태로
		DragVisual->SetPlacementState(false); // 드래그 비주얼에 배치 불가 상태 설정
	}
}

bool UInventoryGridWidget::HandleDropFromScreenPos(UDragDropOperation* Operation, const FVector2D& ScreenPos)
{


	if (!Inventory || !Operation || !CellsLayer) return false;
	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(Operation); // 드래그 오퍼레이션 캐스트
	if (!DragOp || !DragOp->ItemData) return false; // 유효성 검사

	const FVector2D TopLeftScreen = ScreenPos - DragOp->DragLocalOffset; // 드롭할 아이템의 좌측 상단 스크린 좌표 계산

	// 드롭 위치(셀) 계산
	FIntPoint ToCell(0, 0); // 드롭할 셀 좌표 초기화
	{
		const FGeometry Geo = CellsLayer->GetCachedGeometry();
		const FVector2D Local = Geo.AbsoluteToLocal(TopLeftScreen);
		const int32 X = FMath::FloorToInt(Local.X / CellSize);
		const int32 Y = FMath::FloorToInt(Local.Y / CellSize);
		ToCell = FIntPoint(X, Y);
	} // 드롭 위치(셀) 계산 끝

	// 범위 체크(드롭 대상 = 이 그리드의 인벤토리 크기)
	if (ToCell.X < 0 || ToCell.Y < 0 || ToCell.X >= Inventory->Width || ToCell.Y >= Inventory->Height)
	{
		return false;
	}

	bool bResult = false; // 결과 플래그 초기화
	
	if(DragOp->SourceInventory) // 소스 인벤토리가 유효한지 확인
	{
		if (DragOp->SourceInventory == Inventory) {
			bResult = Inventory->MoveItem(DragOp->FromCell, ToCell); // 같은 인벤토리 내에서 이동

		}
		else {
			if(Inventory->AddItemAt(DragOp->ItemData, DragOp->Quantity, ToCell)) // 다른 인벤토리에서 추가 시도
			{

				bResult = DragOp->SourceInventory->RemoveItemAt(DragOp->FromCell); // 소스 인벤토리에서 제거
				bResult = true; // 성공 플래그 설정
			}
		}
	}
	else if (DragOp->SourceWorldItemActor) {
		if(Inventory->AddItemAt(DragOp->ItemData, DragOp->Quantity, ToCell)) // 월드 아이템에서 추가 시도
		{

			if (AQPCharacter* Char = Cast<AQPCharacter>(GetOwningPlayerPawn()))
			{
				if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(DragOp->SourceWorldItemActor))
				{
					int32 FoundSlot = WorldItem->AssignedSlotIndex;
					int32 FoundCode = WorldItem->AssignedCodeNumber;
					Char->ServerDestroyPickupActor(WorldItem, FoundSlot, FoundCode);
				}
				else
				{
					Char->ServerDestroyPickupActor(DragOp->SourceWorldItemActor);
				}
			}
			else
			{
				DragOp->SourceWorldItemActor->Destroy(); // 월드 아이템 액터 파괴 Fallback
			}
			bResult = true; // 성공 플래그 설정
		}
	}
	if(bResult)
	{

		RefreshGrid(); // 그리드 새로고침
	}

	return bResult;
}

bool UInventoryGridWidget::ScreenToCell(const FVector2D& ScreenPos, FIntPoint& OutCell) const
{
	if (!Inventory) return false; // 인벤토리가 유효한지 확인

	const FGeometry Geo = GetCachedGeometry(); // 위젯의 기하학 정보 가져오기
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos); // 화면 좌표를 로컬 좌표로 변환

	const int32 X = FMath::FloorToInt(Local.X / CellSize); // 셀 X 좌표 계산
	const int32 Y = FMath::FloorToInt(Local.Y / CellSize); // 셀 Y 좌표 계산

	if (X < 0 || Y < 0 || X >= Inventory->Width || Y >= Inventory->Height) //	셀 좌표가 인벤토리 범위를 벗어나는지 확인
	{
		return false; // 범위를 벗어나면 false 반환
	}

	OutCell = FIntPoint(X, Y); // 출력 셀 좌표 설정
	return true; // 성공 반환
}

void UInventoryGridWidget::UpdateOccluedCells()
{
	if (!Inventory) return; // 인벤토리가 유효한지 확인
	if (CellWidgets.Num() != Inventory->Width * Inventory->Height) return; // 셀 위젯 배열 크기 확인

	// 1) 전부 보이게
	for (UUserWidget* Cell : CellWidgets) // 모든 셀 위젯 반복
	{
		if (Cell) // 셀 위젯이 유효한지 확인
		{
			Cell->SetVisibility(ESlateVisibility::Visible); // 모든 셀 라인 보이게
		}
	}

	// 2) 아이템 슬롯 돌면서 점유 영역 Hidden
	for (const FInventorySlot& InvSlot : Inventory->Slots) // 인벤토리 슬롯 반복
	{
		if (!InvSlot.Item.ItemData) continue; // 아이템 데이터가 유효하지 않으면 다음으로

		FIntPoint Size = InvSlot.Item.ItemData->ItemSize; // 아이템 크기 가져오기
		Size.X = FMath::Max(1, Size.X); // 최소 크기 1로 설정
		Size.Y = FMath::Max(1, Size.Y); // 최소 크기 1로 설정

		for (int32 dy = 0; dy < Size.Y; ++dy) // 세로 크기 반복
		{
			for (int32 dx = 0; dx < Size.X; ++dx) // 가로 크기 반복
			{
				const int32 X = InvSlot.Position.X + dx; // 점유 칸 X 좌표 계산
				const int32 Y = InvSlot.Position.Y + dy; // 점유 칸 Y 좌표 계산

				if (X < 0 || Y < 0 || X >= Inventory->Width || Y >= Inventory->Height) continue; // 범위 벗어나면 다음으로

				if (UUserWidget* Cell = CellWidgets[CellIndex(X, Y)]) // 셀 위젯 가져오기
				{
					Cell->SetVisibility(ESlateVisibility::Hidden); // 점유된 셀 라인 숨기기
				}
			}
		}
	}
}

bool UInventoryGridWidget::CanPlaceForDragPreview(UInventoryDragOperation* DragOp, const FIntPoint& ToCell) const
{
	if (!Inventory || !DragOp || !DragOp->ItemData) return false; // 유효성 검사

	// 아이템 사이즈
	FIntPoint Size = DragOp->ItemData->ItemSize; // 아이템 크기 가져오기
	Size.X = FMath::Max(1, Size.X); // 최소 크기 1로 설정
	Size.Y = FMath::Max(1, Size.Y); // 최소 크기 1로 설정

	// 1) 바운더리 체크
	if (ToCell.X < 0 || ToCell.Y < 0) return false; // 음수 좌표 체크
	if (ToCell.X + Size.X > Inventory->Width)  return false; // 가로 경계 체크
	if (ToCell.Y + Size.Y > Inventory->Height) return false; // 세로 경계 체크

	// 2) 겹침 체크 (같은 인벤에서 드래그 중이면, 자기 슬롯(FromCell)은 무시)
	const bool bIgnoreSelf = (DragOp->SourceInventory == Inventory) && (DragOp->FromCell.X >= 0 && DragOp->FromCell.Y >= 0); // 자기 자신 무시 여부 결정

	const FIntPoint NewPos = ToCell; // 새 위치

	for (const FInventorySlot& InvSlot : Inventory->Slots) // 인벤토리 슬롯 반복
	{
		if (!InvSlot.Item.ItemData) continue; // 아이템 데이터가 유효하지 않으면 다음으로

		if (bIgnoreSelf && InvSlot.Position == DragOp->FromCell) // 자기 자신 무시 조건 검사
		{
			continue; //자기 자신은 무시
		}

		FIntPoint ExistSize = InvSlot.Item.ItemData->ItemSize; // 기존 아이템 크기 가져오기
		ExistSize.X = FMath::Max(1, ExistSize.X); // 최소 크기 1로 설정
		ExistSize.Y = FMath::Max(1, ExistSize.Y); // 최소 크기 1로 설정

		const FIntPoint ExistPos = InvSlot.Position; // 기존 아이템 위치
		// 사각형 겹침 검사
		const bool bOverlapX = NewPos.X < ExistPos.X + ExistSize.X && NewPos.X + Size.X > ExistPos.X; // X축 겹침 검사
		const bool bOverlapY = NewPos.Y < ExistPos.Y + ExistSize.Y && NewPos.Y + Size.Y > ExistPos.Y; // Y축 겹침 검사
		if (bOverlapX && bOverlapY) // 겹침 발견
		{
			return false;
		}
	}

	return true;
}
