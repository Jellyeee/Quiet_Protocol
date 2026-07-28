#include "PJ_Quiet_Protocol/GameMode/QPMainMenuGameMode.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"

AQPMainMenuGameMode::AQPMainMenuGameMode()
{
	// 플레이어 캐릭터가 스폰되지 않도록 설정
	DefaultPawnClass = nullptr;
}

void AQPMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (MainMenuBGM)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlayBGM(MainMenuBGM);
			}
		}
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && MainMenuWidgetClass)
	{
		// 1. 위젯 생성 및 화면 추가
		MainMenuWidget = CreateWidget<UUserWidget>(PC, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
		}

		// 2. 입력 모드 설정 및 커서 표시
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		
		// 3. 맵에 배치된 카메라를 찾아서 화면(배경)으로 설정
		TArray<AActor*> FoundCameras;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);
		if (FoundCameras.Num() > 0)
		{
			// 가장 첫 번째로 찾은 카메라를 배경으로 씁니다
			PC->SetViewTargetWithBlend(FoundCameras[0]);
		}
	}
}
