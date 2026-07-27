#include "InventoryRootWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "InventoryDragOperation.h" 
#include "Components/TextBlock.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"

void UInventoryRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	/** 초기 생성 시 소유 캐릭터의 인벤토리를 찾아 초기화합니다. */
	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn());
	if (Character && Character->GetInventoryComponent())
	{
		InitializeInventory(Character->GetInventoryComponent());
	}
}

void UInventoryRootWidget::InitializeInventory(UInventoryComponent* NewInventory)
{
	if (!NewInventory || !InventoryGrid) return;

	/** 기존 인벤토리와의 바인딩이 있다면 해제합니다. */
	if (CachedInventory)
	{
		CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryRootWidget::HandleInventoryChanged);
	}

	/** 새로운 인벤토리를 캐싱하고 델리게이트를 바인딩합니다. */
	CachedInventory = NewInventory;
	CachedInventory->OnInventoryChanged.AddUniqueDynamic(this, &UInventoryRootWidget::HandleInventoryChanged);

	/** 그리드 위젯에도 새로운 인벤토리를 전달하고 갱신합니다. */
	InventoryGrid->SetInventory(CachedInventory);
	InventoryGrid->RefreshGrid();

	/** 상단 비밀번호 정보를 갱신합니다. */
	UpdatePasswordDisplay();
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
	UpdatePasswordDisplay();
}

void UInventoryRootWidget::UpdatePasswordDisplay()
{
	if (!PasswordText) return;

	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn());
	if (!Character) return;

	TArray<int32> Collected = Character->GetCollectedPassword();

	// 배열이 비어있다면 GameState에서 받아와서 임시 채움
	if (Collected.Num() == 0)
	{
		if (AQPEscapeGameState* GS = GetWorld()->GetGameState<AQPEscapeGameState>())
		{
			if (GS->TotalGenerators > 0)
			{
				Collected.Init(-1, GS->TotalGenerators);
			}
		}
	}

	FString DisplayString;

	// 인벤토리 상단에 표시될 문자열 생성 (예: "_ _ 3 _")
	for (int32 i = 0; i < Collected.Num(); ++i)
	{
		if (Collected[i] == -1)
		{
			DisplayString += TEXT("_");
		}
		else
		{
			DisplayString += FString::FromInt(Collected[i]);
		}

		if (i < Collected.Num() - 1)
		{
			DisplayString += TEXT(" ");
		}
	}

	PasswordText->SetText(FText::FromString(DisplayString));
}
