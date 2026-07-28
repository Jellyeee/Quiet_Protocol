#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "QPLobbyPlayerController.generated.h"

/**
 * 로비 맵 전용 플레이어 컨트롤러
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 블루프린트에서 위젯 클래스를 할당할 수 있도록 노출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> LobbyWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class UUserWidget* LobbyWidgetInstance;
};
