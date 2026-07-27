#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPOptionsWidget.generated.h"

/**
 * 게임 옵션 설정 UI의 베이스 클래스
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 변경된 설정을 시스템에 적용 */
	UFUNCTION(BlueprintCallable, Category = "QP|UI|Menu")
	void ApplySettings();

	/** 옵션 창 닫기 */
	UFUNCTION(BlueprintCallable, Category = "QP|UI|Menu")
	void CloseOptions();

	/** 오디오 볼륨 설정 (슬라이더 조절 시 실시간 반영용) */
	UFUNCTION(BlueprintCallable, Category = "QP|UI|Audio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "QP|UI|Audio")
	void SetBGMVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "QP|UI|Audio")
	void SetSFXVolume(float Volume);

	/** 현재 등록된 오디오 볼륨 가져오기 (UI 초기화용) */
	UFUNCTION(BlueprintPure, Category = "QP|UI|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "QP|UI|Audio")
	float GetBGMVolume() const;

	UFUNCTION(BlueprintPure, Category = "QP|UI|Audio")
	float GetSFXVolume() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> ClickSound;

	UFUNCTION(BlueprintCallable, Category = "QP|UI|Audio")
	void PlayClickSound();

	/** 설정이 적용되었을 때 Blueprint에서 호출될 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "QP|UI|Menu")
	void OnSettingsApplied();

	/** 옵션 위젯이 닫힐 때 Blueprint에서 호출될 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "QP|UI|Menu")
	void OnOptionsClosed();
};
