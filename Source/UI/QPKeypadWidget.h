#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPKeypadWidget.generated.h"

class AQPEscapeDoor;
class UTextBlock;
class UButton;

/**
 * UQPKeypadWidget
 * 탈출문에 상호작용할 때 나타나는 비밀번호 입력 UI입니다.
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPKeypadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 위젯을 띄운 탈출문을 기록해둡니다.
	void SetTargetDoor(AQPEscapeDoor* InDoor) { TargetDoor = InDoor; }

protected:
	virtual void NativeConstruct() override;

	// 비밀번호 패널에 보여질 텍스트 
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PasswordDisplayText;

	// 번호판 버튼들
	UFUNCTION(BlueprintCallable, Category = "Keypad")
	void OnNumberClicked(int32 Number);

	// 지우기 버튼
	UFUNCTION(BlueprintCallable, Category = "Keypad")
	void OnEraseClicked();

	// 입력 완료 버튼
	UFUNCTION(BlueprintCallable, Category = "Keypad")
	void OnSubmitClicked();

	// 닫기 버튼
	UFUNCTION(BlueprintCallable, Category = "Keypad")
	void OnCloseClicked();

private:
	// 타겟 탈출문
	UPROPERTY()
	AQPEscapeDoor* TargetDoor;

	// 현재까지 누른 번호 배열 (비밀번호 대조용)
	UPROPERTY()
	TArray<int32> InputDigits;

	// 텍스트 패널에 보여질 상태를 갱신
	void UpdateDisplay();
};
