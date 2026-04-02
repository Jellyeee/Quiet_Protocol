#include "ItemIconWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "InventoryDragOperation.h"
#include "ItemDragVisualWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "InventoryContextMenuWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UItemIconWidget::Setup(UInventoryComponent* InInventory, UItemDataAsset* InItemData, int32 InQuantity, const FIntPoint& InFrom, const FIntPoint& InItemSize, float InCellSize, TSubclassOf<UUserWidget> InDragVisualClass, UInventoryGridWidget* InOwningGrid)
{
	Inventory = InInventory;
	ItemData = InItemData;
	Quantity = InQuantity;
	From = InFrom;

	ItemSize = InItemSize;
	ItemSize.X = FMath::Max(1, ItemSize.X);
	ItemSize.Y = FMath::Max(1, ItemSize.Y);

	CellSize = InCellSize;
	DragVisualClass = InDragVisualClass;
	OwningGrid = InOwningGrid;

	ApplyVisual();
}

FReply UItemIconWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		if (!ContextMenuClass) return FReply::Handled();

		if (OpenedMenu)
		{
			OpenedMenu->RemoveFromParent();
			OpenedMenu = nullptr;
		}

		OpenedMenu = CreateWidget<UInventoryContextMenuWidget>(GetOwningPlayer(), ContextMenuClass);
		if (!OpenedMenu) return FReply::Handled();

		OpenedMenu->InitMenu(From);

		OpenedMenu->OnEquip.BindLambda([this](const FIntPoint& Cell)
			{
				if (AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn()))
				{
					Character->EquipInventoryItemAt(Cell);
				}
			});

		OpenedMenu->OnDrop.BindLambda([this](const FIntPoint& Cell)
			{
				if (AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn()))
				{
					Character->DropInventoryItemAt(Cell);
				}
			});

		OpenedMenu->AddToViewport(9999);

		APlayerController* PlayerController = GetOwningPlayer();
		if (!PlayerController) return FReply::Handled();
		float X = 0.f, Y = 0.f;
		if(PlayerController->GetMousePosition(X, Y))
		{
			const FVector2D MousePosition(X, Y);
			OpenedMenu->SetPositionInViewport(MousePosition, false);
		}

		OpenedMenu->SetKeyboardFocus();
		return FReply::Handled();
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemIconWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!Inventory || !ItemData) return;

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragOperation::StaticClass())
	);
	if (!DragOp) return;

	DragOp->SourceInventory = Inventory;
	DragOp->FromCell = From;
	DragOp->ItemData = ItemData;
	DragOp->Quantity = Quantity;

	DragOp->DragLocalOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	DragOp->ItemPixelSize = FVector2D(ItemSize.X * CellSize, ItemSize.Y * CellSize);
	if (DragVisualClass)
	{
		UUserWidget* Visual = CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass);
		if (Visual)
		{
			Visual->SetDesiredSizeInViewport(DragOp->ItemPixelSize);
			if (UItemDragVisualWidget* DragVisual = Cast<UItemDragVisualWidget>(Visual))
			{
				DragVisual->SetVisual(ItemData, Quantity, DragOp->ItemPixelSize);
			}

			DragOp->DefaultDragVisual = Visual;
		}
	}

	DragOp->Pivot = EDragPivot::MouseDown;
	OutOperation = DragOp;
}


bool UItemIconWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (OwningGrid)
	{
		const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();
		return OwningGrid->HandleDropFromScreenPos(InOperation, ScreenPos);
	}
	return false;
}

void UItemIconWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
}

void UItemIconWidget::ApplyVisual()
{
	if (ItemImage && ItemData && ItemData->ItemIcon)
	{
		ItemImage->SetBrushFromTexture(ItemData->ItemIcon);
	}

	if (QuantityText)
	{
		if (Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetText(FText::GetEmpty());
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
