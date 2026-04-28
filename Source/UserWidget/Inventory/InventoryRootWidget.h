#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryRootWidget.generated.h"


UCLASS()
class PJ_QUIET_PROTOCOL_API UInventoryRootWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 인벤토리 컴포넌트를 전달받아 UI를 초기화하거나 갱신합니다. (캐릭터 리스폰 시 호출됨) */
	void InitializeInventory(class UInventoryComponent* NewInventory);

protected:
	virtual void NativeConstruct() override; // 위젯이 생성될때 불림
	virtual void NativeDestruct() override; // 위젯이 파괴될때 불림
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override; // 드롭 처리
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UInventoryGridWidget> InventoryGrid; // 인벤토리 그리드 위젯 바인딩

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> PasswordText;

	UFUNCTION()
	void UpdatePasswordDisplay(); // 비밀번호 텍스트 업데이트

private:
	UPROPERTY()
	TObjectPtr<class UInventoryComponent> CachedInventory = nullptr; // 캐시된 인벤토리 컴포넌트

	UFUNCTION()
	void HandleInventoryChanged(); // 인벤토리 변경 핸들러
};
