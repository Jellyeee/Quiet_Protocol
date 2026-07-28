#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "QPMainMenuGameMode.generated.h"

/**
 * 메인 메뉴 전용 게임 모드
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AQPMainMenuGameMode();

protected:
	virtual void BeginPlay() override;

	/** 메인메뉴 BGM 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> MainMenuBGM;

	/** 생성할 메인 메뉴 위젯 클래스 (Blueprint에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|UI")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

private:
	/** 생성된 위젯 인스턴스 보관 */
	UPROPERTY()
	class UUserWidget* MainMenuWidget;
};
