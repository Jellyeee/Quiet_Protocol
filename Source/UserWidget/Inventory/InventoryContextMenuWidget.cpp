#include "InventoryContextMenuWidget.h"
#include "Components/Button.h"

void UInventoryContextMenuWidget::InitMenu(const FIntPoint& InCell) // 초기화 함수
{
	Cell = InCell; // 선택된 셀 위치 설정
}

void UInventoryContextMenuWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모 클래스의 NativeConstruct 호출

	SetIsFocusable(true); // 위젯을 포커스 가능하게 설정

	// 버튼들이 포커스를 뺏어가서 메뉴가 닫히는 현상(클릭 무시)을 방지
	if (BtnEquip) 
	{
		BtnEquip->IsFocusable = false;
		BtnEquip->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnEquip); 
	}
	if (BtnDrop)  
	{
		BtnDrop->IsFocusable = false;
		BtnDrop->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnDrop);   
	}
	if (BtnClose) 
	{
		BtnClose->IsFocusable = false;
		BtnClose->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnClose); 
	}
}

void UInventoryContextMenuWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent); // 부모 클래스의 NativeOnFocusLost 호출
	
	// 마우스가 메뉴(혹은 버튼) 위에 있다면 버튼을 클릭한 것이므로 닫지 않음
	if (IsHovered())
	{
		return;
	}

	// 다른 곳(허공이나 다른 아이템)을 클릭하면 메뉴를 자동으로 닫음
	RemoveFromParent();
}

void UInventoryContextMenuWidget::OnClickedBtnEquip()
{
	if(OnEquip.IsBound()) OnEquip.Execute(Cell); // 아이템 사용 델리게이트 실행
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}

void UInventoryContextMenuWidget::OnClickedBtnDrop()
{
	if (OnDrop.IsBound()) OnDrop.Execute(Cell); // 아이템 버리기 델리게이트 실행
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}

void UInventoryContextMenuWidget::OnClickedBtnClose()
{
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}
