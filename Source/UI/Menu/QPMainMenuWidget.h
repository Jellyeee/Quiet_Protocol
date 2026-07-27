#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "QPServerListEntryWidget.h"
#include "QPMainMenuWidget.generated.h"

/**
 * 메인 메뉴 UI의 베이스 클래스
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 위젯 블루프린트의 버튼들과 이름이 일치해야 합니다. */
	UPROPERTY(meta = (BindWidget))
	class UButton* Host;

	UPROPERTY(meta = (BindWidget))
	class UButton* Join;

	UPROPERTY(meta = (BindWidget))
	class UButton* Option;

	UPROPERTY(meta = (BindWidget))
	class UButton* Quit;

public:
	/** 방 만들기(호스트) 버튼 클릭 시 호출 */
	UFUNCTION()
	void OnHostButtonClicked();

	/** 참가하기 버튼 클릭 시 호출 */
	UFUNCTION()
	void OnJoinButtonClicked();

	/** 옵션 버튼 클릭 시 호출 */
	UFUNCTION()
	void OnOptionsButtonClicked();

	/** 게임 종료 버튼 클릭 시 호출 */
	UFUNCTION()
	void OnQuitButtonClicked();

protected:
	/** 방 생성 완료 시 호출되는 콜백 */
	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);

	/** 방 검색 완료 시 호출되는 콜백 */
	UFUNCTION()
	void OnFindSessionsComplete(bool bWasSuccessful);

	/** 방 참가 완료 시 호출되는 콜백 */
	UFUNCTION()
	void OnJoinSessionComplete(bool bWasSuccessful);

	/** 새로고침 버튼 클릭 시 호출 */
	UFUNCTION()
	void OnRefreshButtonClicked();

protected:
	/** 옵션 메뉴를 화면에 표시 (Blueprint에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "QP|UI|Menu")
	void ShowOptionsWidget();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	class UScrollBox* ServerListScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* RefreshButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* ServerListContainer;

	UPROPERTY(EditDefaultsOnly, Category = "QP|UI|Menu")
	TSubclassOf<class UUserWidget> ServerListEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "QP|UI|Menu")
	TSubclassOf<class UUserWidget> OptionsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> ClickSound;

	void PlayClickSound();
};
