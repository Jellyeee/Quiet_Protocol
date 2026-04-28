#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPGeneratorWidget.generated.h"

class UProgressBar;

/**
 * 발전기 수리와 관련된 게이지 바 및 스킬체크 UI 베이스
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPGeneratorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 수리 달성도(0.0 ~ 1.0)를 전달하여 프로그레스 바 갱신
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Escape|UI")
	void UpdateRepairProgress(float Ratio);

	// 스킬 체크가 시작되었을 때 호출 (표적 구간 위치와 너비)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Escape|UI")
	void ShowSkillCheckPopup(float TargetStartRatio, float TargetWidthRatio, float DurationSeconds);

	// 스킬 체크 팝업 닫기 (성공/실패 시점)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Escape|UI")
	void HideSkillCheckPopup();
};
