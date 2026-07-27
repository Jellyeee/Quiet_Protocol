#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPPauseMenuWidget.generated.h"

class UButton;
class UQPOptionsWidget;

/**
 * 인게임 ESC(일시정지) 메뉴 위젯
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 버튼 위젯 바인딩 (이름은 에디터와 동일하게 맞춰야 함)
	UPROPERTY(meta = (BindWidget))
	UButton* ResumeButton;

	UPROPERTY(meta = (BindWidget))
	UButton* OptionButton;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	// 옵션 위젯 인스턴스를 담을 컨테이너 (Blueprint에서 추가할 수 있도록)
	UPROPERTY(meta = (BindWidgetOptional))
	class UOverlay* OptionOverlay;

	// 옵션 위젯 클래스 설정용
	UPROPERTY(EditDefaultsOnly, Category = "QP|UI|PauseMenu")
	TSubclassOf<UQPOptionsWidget> OptionWidgetClass;

private:
	// 생성된 옵션 위젯 캐싱
	UPROPERTY()
	UQPOptionsWidget* OptionWidgetInstance;

	// 버튼 클릭 이벤트 처리 함수들
	UFUNCTION()
	void OnResumeButtonClicked();

	UFUNCTION()
	void OnOptionButtonClicked();

	UFUNCTION()
	void OnQuitButtonClicked();

	// 세션 파괴 완료 콜백
	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);

public:
	// 컨트롤러에서 메뉴를 닫을 때 호출하는 유틸리티 함수
	void CloseMenu();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> ClickSound;

	void PlayClickSound();
};
