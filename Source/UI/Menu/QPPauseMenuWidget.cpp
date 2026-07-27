#include "QPPauseMenuWidget.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "PJ_Quiet_Protocol/UI/Menu/QPOptionsWidget.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "PJ_Quiet_Protocol/Character/Controllers/QPPlayerController.h"
#include "PJ_Quiet_Protocol/Session/QPSessionSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UQPPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UQPPauseMenuWidget::OnResumeButtonClicked);
	}

	if (OptionButton)
	{
		OptionButton->OnClicked.AddDynamic(this, &UQPPauseMenuWidget::OnOptionButtonClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UQPPauseMenuWidget::OnQuitButtonClicked);
	}
}

void UQPPauseMenuWidget::PlayClickSound()
{
	if (ClickSound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlayUISound(ClickSound);
			}
		}
	}
}

void UQPPauseMenuWidget::OnResumeButtonClicked()
{
	PlayClickSound();

	// 계속하기 버튼 클릭 시 창 닫기 및 마우스 숨김
	CloseMenu();
}

void UQPPauseMenuWidget::OnOptionButtonClicked()
{
	PlayClickSound();

	// 메인메뉴와 동일하게 뷰포트 전체 화면(Z-Order 200)에 직접 띄우기
	if (!OptionWidgetInstance && OptionWidgetClass)
	{
		OptionWidgetInstance = CreateWidget<UQPOptionsWidget>(GetWorld(), OptionWidgetClass);
	}

	if (OptionWidgetInstance)
	{
		if (!OptionWidgetInstance->IsInViewport())
		{
			OptionWidgetInstance->AddToViewport(200);
		}

		if (OptionWidgetInstance->GetVisibility() == ESlateVisibility::Visible)
		{
			OptionWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			OptionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("[PauseMenu] OptionWidgetClass is not set in Blueprint!"));
	}
}

void UQPPauseMenuWidget::OnQuitButtonClicked()
{
	PlayClickSound();

	// 버튼 연속 클릭 방지
	QuitButton->SetIsEnabled(false);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UQPSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
		{
			// 멀티플레이 세션 파괴 이벤트 구독
			SessionSubsystem->OnDestroySessionCompleteEvent.AddDynamic(this, &UQPPauseMenuWidget::OnDestroySessionComplete);
			
			// 세션 파괴 호출
			SessionSubsystem->DestroySession();
			return; // 콜백에서 이동 처리
		}
	}

	// 세션 서브시스템을 못 찾았거나 실패 시, 예비용으로 강제 이동 처리
	OnDestroySessionComplete(false);
}

void UQPPauseMenuWidget::OnDestroySessionComplete(bool bWasSuccessful)
{
	// 메인 메뉴 맵으로 강제 이동
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel("/Game/Maps/Main", ETravelType::TRAVEL_Absolute);
	}
	else
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName("Main"));
	}
}

void UQPPauseMenuWidget::CloseMenu()
{
	if (AQPPlayerController* QPController = Cast<AQPPlayerController>(GetOwningPlayer()))
	{
		// 플레이어 컨트롤러에서 토글 함수를 호출하여 완벽하게 끄기
		QPController->TogglePauseMenu();
	}
	else
	{
		// 컨트롤러를 캐스팅하지 못했을 경우의 안전장치
		RemoveFromParent();
		if (APlayerController* PC = GetOwningPlayer())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
		}
	}
}
