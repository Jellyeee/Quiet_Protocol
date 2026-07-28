#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPLobbyWidget.generated.h"

/**
 * 로비 맵의 화면 UI
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// '게임 시작' 버튼. 블루프린트 위젯의 이름과 반드시 같아야 합니다.
	UPROPERTY(meta = (BindWidget))
	class UButton* StartGameButton;

	// 플레이어 목록을 담을 컨테이너
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* PlayerListContainer;

	// 로비 나가기(방 파괴) 버튼
	UPROPERTY(meta = (BindWidget))
	class UButton* QuitLobbyButton;

	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void UpdatePlayerList();

	UFUNCTION()
	void OnQuitLobbyClicked();

	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);

private:
	FTimerHandle UpdatePlayerListTimerHandle;
};
