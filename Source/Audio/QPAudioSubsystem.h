#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QPAudioSubsystem.generated.h"

class USoundClass;
class UAudioComponent;
class USoundBase;

/**
 * QPAudioSubsystem
 * 
 * 게임 전체의 오디오 볼륨(마스터, BGM, SFX) 및 배경음악(BGM) 재생을 총괄하는 전역 서브시스템.
 * UGameInstanceSubsystem을 상속받아 맵 이동(ServerTravel / OpenLevel) 시에도 파괴되지 않고 연속성을 유지합니다.
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 마스터 볼륨 설정 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetMasterVolume(float Volume);

	/** BGM 볼륨 설정 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetBGMVolume(float Volume);

	/** SFX 볼륨 설정 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetSFXVolume(float Volume);

	/** 현재 볼륨 반환 */
	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetSFXVolume() const { return SFXVolume; }

	/** BGM 재생 (동일한 BGM 재생 중이면 중복 실행 방지) */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayBGM(USoundBase* NewBGM, bool bFadeIn = true, float FadeDuration = 1.0f);

	/** BGM 페이드아웃 정지 */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void StopBGM(bool bFadeOut = true, float FadeDuration = 1.0f);

	/** UI 2D 효과음 재생 */
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayUISound(USoundBase* Sound);

	/** 3D 위치 효과음 재생 (SFX * Master 실시간 볼륨 자동 적용) */
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySoundAtLocation(USoundBase* Sound, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	/** 설정 저장 및 불러오기 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Save")
	void SaveAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Save")
	void LoadAudioSettings();

	/** 메모리에 로드된 모든 SFX SoundClass에 저장된 실시간 볼륨 수치를 강제 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void EnsureSoundClassVolumesApplied();

	/** SoundClass 수동 연결 (선택 사항) */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetSoundClasses(USoundClass* InMaster, USoundClass* InBGM, USoundClass* InSFX);

private:
	void ApplyVolume(USoundClass* TargetClass, float Volume);

	UPROPERTY()
	float MasterVolume = 1.0f;

	UPROPERTY()
	float BGMVolume = 1.0f;

	UPROPERTY()
	float SFXVolume = 1.0f;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BGMComponent;

	UPROPERTY()
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> BGMSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> SFXSoundClass;
};
