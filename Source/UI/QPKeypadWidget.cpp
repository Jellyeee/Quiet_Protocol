#include "PJ_Quiet_Protocol/UI/QPKeypadWidget.h"
#include "Components/TextBlock.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeDoor.h"
#include "Kismet/GameplayStatics.h"

void UQPKeypadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InputDigits.Empty();
	UpdateDisplay();

	// UI 모드로 전환하고 마우스 커서 표시
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UQPKeypadWidget::OnNumberClicked(int32 Number)
{
	// 너무 많이 입력 방지 (최대 10개)
	if (InputDigits.Num() < 10)
	{
		InputDigits.Add(Number);
		UpdateDisplay();
	}
}

void UQPKeypadWidget::OnEraseClicked()
{
	if (InputDigits.Num() > 0)
	{
		InputDigits.Pop();
		UpdateDisplay();
	}
}

void UQPKeypadWidget::OnSubmitClicked()
{
	if (TargetDoor)
	{
		TargetDoor->SubmitPassword(InputDigits);
	}

	// 제출 후에는 창을 닫음 (입력 모드 초기화 포함)
	OnCloseClicked();
}

void UQPKeypadWidget::OnCloseClicked()
{
	// 플레이어 모드로 되돌림
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	// 위젯 제거
	RemoveFromParent();
}

void UQPKeypadWidget::UpdateDisplay()
{
	if (PasswordDisplayText)
	{
		FString DisplayStr = TEXT("");
		for (int32 Digit : InputDigits)
		{
			DisplayStr += FString::FromInt(Digit) + TEXT(" ");
		}
		
		if (DisplayStr.IsEmpty())
		{
			DisplayStr = TEXT("비밀번호 입력...");
		}

		PasswordDisplayText->SetText(FText::FromString(DisplayStr));
	}
}
