#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "QPSettingsSaveGame.generated.h"

/**
 * 게임 설정을 파일로 저장하기 위한 클래스
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UQPSettingsSaveGame();

	/** 마스터 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float MasterVolume;

	/** BGM 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float BGMVolume;

	/** SFX 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float SFXVolume;

	/** 마우스 감도 */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity;

	/** 해상도 스케일 */
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	int32 ResolutionQuality;

	/** 슬롯 이름 */
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FString SaveSlotName;

	/** 유저 인덱스 */
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	int32 UserIndex;
};
