#include "QPLobbyGameMode.h"
#include "PJ_Quiet_Protocol/GameMode/QPLobbyPlayerController.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"

AQPLobbyGameMode::AQPLobbyGameMode()
{
	PlayerControllerClass = AQPLobbyPlayerController::StaticClass();
	bUseSeamlessTravel = true; // 무거운 맵(InGame) 로딩 중 타임아웃 튕김을 방지하기 위해 심리스 트래블 활성화
}

void AQPLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (LobbyBGM)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlayBGM(LobbyBGM);
			}
		}
	}
}
