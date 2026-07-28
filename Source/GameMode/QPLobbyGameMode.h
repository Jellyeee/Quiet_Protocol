#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "QPLobbyGameMode.generated.h"

/**
 * 로비 맵 전용 게임 모드
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AQPLobbyGameMode();

protected:
	virtual void BeginPlay() override;

	/** 로비 BGM 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> LobbyBGM;
};
