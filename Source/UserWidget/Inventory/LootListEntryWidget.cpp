#include "LootListEntryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "InventoryDragOperation.h"
#include "ItemDragVisualWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
void ULootListEntryWidget::Setup(AActor* InSourceActor, UItemDataAsset* InItemData, int32 InQuantity)
{
	SourceActor = InSourceActor; // 출처 액터 설정
	ItemData = InItemData; // 아이템 데이터 설정
	Quantity = InQuantity; // 수량 설정

	if (IconImage && ItemData && ItemData->ItemIcon) // 아이콘 이미지 설정
	{ 
		IconImage->SetBrushFromTexture(ItemData->ItemIcon); // 아이템 아이콘으로 브러시 설정
	}

	if (NameText && ItemData) // 이름 텍스트 설정
	{
		NameText->SetText(ItemData->ItemName); // 아이템 이름으로 텍스트 설정
	}

	if (SizeText && ItemData) // 크기 텍스트 설정
	{
		const int32 W = FMath::Max(1, ItemData->ItemSize.X); // 아이템 너비 계산
		const int32 H = FMath::Max(1, ItemData->ItemSize.Y); // 아이템 높이 계산
		SizeText->SetText(FText::FromString(FString::Printf(TEXT("%dx%d"), W, H))); // 크기 텍스트 설정
	}
}
FReply ULootListEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) // 왼쪽 마우스 버튼이 눌렸는지 확인
	{
		if (InMouseEvent.IsShiftDown()) // 쉬프트 키가 같이 눌렸는지 확인 (빠른 줍기)
		{
			if (SourceActor.IsValid())
			{
				if (AQPCharacter* PlayerChar = Cast<AQPCharacter>(GetOwningPlayerPawn()))
				{
					PlayerChar->QuickLootItem(SourceActor.Get());
					return FReply::Handled();
				}
			}
		}
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply; // 드래그 감지 시작
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent); // 기본 동작 호출
}

void ULootListEntryWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation); // 기본 동작 호출

	if (!SourceActor.IsValid() || !ItemData) return; // 유효성 검사

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragOperation::StaticClass())); // 드래그 드롭 오퍼레이션 생성
	if (!DragOp) return;

	DragOp->ItemData = ItemData; // 아이템 데이터 설정
	DragOp->Quantity = Quantity; // 수량 설정

	DragOp->SourceInventory = nullptr; // 출처 인벤토리 설정(없음)
	DragOp->FromCell = FIntPoint(-1, -1); // 출처 셀 설정(없음)
	DragOp->SourceWorldItemActor = SourceActor.Get(); // 출처 월드 아이템 액터 설정

	DragOp->DragLocalOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()); // 드래그 로컬 오프셋 설정

	const int32 W = FMath::Max(1, ItemData->ItemSize.X); // 아이템 너비 계산
	const int32 H = FMath::Max(1, ItemData->ItemSize.Y); // 아이템 높이 계산
	DragOp->ItemPixelSize = FVector2D(W * CellSize, H * CellSize); // 아이템 픽셀 크기 설정

	if (DragVisualClass) // 드래그 비주얼 클래스가 설정되어 있는지 확인
	{
		UUserWidget* Visual = CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass); // 드래그 비주얼 위젯 생성
		if (Visual)
		{
			if (UItemDragVisualWidget* DragVisual = Cast<UItemDragVisualWidget>(Visual)) // 드래그 비주얼 위젯으로 캐스팅
			{
				DragVisual->SetVisual(ItemData, Quantity, DragOp->ItemPixelSize); // 비주얼 설정
			}

			DragOp->DefaultDragVisual = Visual; // 기본 드래그 비주얼 설정
		}
	}

	DragOp->Pivot = EDragPivot::MouseDown; // 드래그 피벗 설정
	OutOperation = DragOp; // 출력 오퍼레이션 설정
}
