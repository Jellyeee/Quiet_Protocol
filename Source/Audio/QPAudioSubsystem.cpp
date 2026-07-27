#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundBase.h"
#include "PJ_Quiet_Protocol/Commons/QPSettingsSaveGame.h"

void UQPAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadAudioSettings();
}

void UQPAudioSubsystem::Deinitialize()
{
	if (BGMComponent && BGMComponent->IsPlaying())
	{
		BGMComponent->Stop();
	}
	Super::Deinitialize();
}

void UQPAudioSubsystem::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolume(MasterSoundClass, MasterVolume);
	ApplyVolume(SFXSoundClass, SFXVolume * MasterVolume);
	if (BGMComponent)
	{
		BGMComponent->SetVolumeMultiplier(BGMVolume * MasterVolume);
	}
	SaveAudioSettings();
}

void UQPAudioSubsystem::SetBGMVolume(float Volume)
{
	BGMVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolume(BGMSoundClass, BGMVolume);
	if (BGMComponent)
	{
		BGMComponent->SetVolumeMultiplier(BGMVolume * MasterVolume);
	}
	SaveAudioSettings();
}

void UQPAudioSubsystem::SetSFXVolume(float Volume)
{
	SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolume(SFXSoundClass, SFXVolume * MasterVolume);
	SaveAudioSettings();
}

void UQPAudioSubsystem::ApplyVolume(USoundClass* TargetClass, float Volume)
{
	if (TargetClass)
	{
		TargetClass->Properties.Volume = Volume;
	}

	// 프로젝트 메모리에 로드된 모든 SC_SFX 및 SFX SoundClass를 동적으로 검색하여 실시간 적용
	for (TObjectIterator<USoundClass> It; It; ++It)
	{
		USoundClass* SoundClass = *It;
		if (SoundClass && (SoundClass->GetName().Contains(TEXT("SC_SFX")) || SoundClass->GetName().Contains(TEXT("SFX"))))
		{
			SoundClass->Properties.Volume = Volume;
		}
	}
}

void UQPAudioSubsystem::PlayBGM(USoundBase* NewBGM, bool bFadeIn, float FadeDuration)
{
	if (!NewBGM)
	{
		StopBGM(bFadeIn, FadeDuration);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 이미 동일한 BGM이 재생 중인 경우 재생을 유지시킴
	if (BGMComponent && BGMComponent->IsPlaying() && BGMComponent->Sound == NewBGM)
	{
		return;
	}

	// 기존 BGM이 있다면 정지
	if (BGMComponent)
	{
		BGMComponent->Stop();
	}

	float EffectiveVolume = BGMVolume * MasterVolume;
	if (EffectiveVolume <= 0.001f)
	{
		// 세이브 파일 0.0f 버그 방지 - 기본 볼륨 1.0f로 보정
		EffectiveVolume = 1.0f;
	}

	// 2D 사운드 스폰 및 명시적 Play() 호출 (MetaSound / SoundWave / SoundCue 100% 호환)
	BGMComponent = UGameplayStatics::SpawnSound2D(World, NewBGM, EffectiveVolume, 1.0f, 0.0f, nullptr, true, true);
	if (BGMComponent)
	{
		BGMComponent->Play();
		if (bFadeIn && FadeDuration > 0.0f)
		{
			BGMComponent->FadeIn(FadeDuration, EffectiveVolume);
		}
	}

	if (GEngine && NewBGM)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("[Audio] PlayBGM Executed: %s (Volume: %.2f)"), *NewBGM->GetName(), EffectiveVolume));
	}
}

void UQPAudioSubsystem::StopBGM(bool bFadeOut, float FadeDuration)
{
	if (BGMComponent && BGMComponent->IsPlaying())
	{
		if (bFadeOut)
		{
			BGMComponent->FadeOut(FadeDuration, 0.0f);
		}
		else
		{
			BGMComponent->Stop();
		}
	}
}

void UQPAudioSubsystem::PlayUISound(USoundBase* Sound)
{
	if (!Sound) return;
	float EffectiveVolume = SFXVolume * MasterVolume;
	UGameplayStatics::PlaySound2D(GetWorld(), Sound, EffectiveVolume);
}

void UQPAudioSubsystem::PlaySoundAtLocation(USoundBase* Sound, FVector Location, FRotator Rotation)
{
	if (!Sound) return;
	float EffectiveVolume = SFXVolume * MasterVolume;
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location, Rotation, EffectiveVolume);
}

void UQPAudioSubsystem::SetSoundClasses(USoundClass* InMaster, USoundClass* InBGM, USoundClass* InSFX)
{
	MasterSoundClass = InMaster;
	BGMSoundClass = InBGM;
	SFXSoundClass = InSFX;

	EnsureSoundClassVolumesApplied();
}

void UQPAudioSubsystem::EnsureSoundClassVolumesApplied()
{
	ApplyVolume(MasterSoundClass, MasterVolume);
	ApplyVolume(BGMSoundClass, BGMVolume * MasterVolume);
	ApplyVolume(SFXSoundClass, SFXVolume * MasterVolume);
}

void UQPAudioSubsystem::SaveAudioSettings()
{
	FString SaveSlotName = TEXT("QPSettingsSaveSlot");
	int32 UserIndex = 0;

	UQPSettingsSaveGame* SaveGameInstance = Cast<UQPSettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

	if (!SaveGameInstance)
	{
		SaveGameInstance = Cast<UQPSettingsSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UQPSettingsSaveGame::StaticClass()));
	}

	if (SaveGameInstance)
	{
		SaveGameInstance->MasterVolume = MasterVolume;
		SaveGameInstance->BGMVolume = BGMVolume;
		SaveGameInstance->SFXVolume = SFXVolume;

		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveSlotName, UserIndex);
	}
}

void UQPAudioSubsystem::LoadAudioSettings()
{
	FString SaveSlotName = TEXT("QPSettingsSaveSlot");
	int32 UserIndex = 0;

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		UQPSettingsSaveGame* SaveGameInstance = Cast<UQPSettingsSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

		if (SaveGameInstance)
		{
			MasterVolume = SaveGameInstance->MasterVolume;
			BGMVolume = SaveGameInstance->BGMVolume;
			SFXVolume = SaveGameInstance->SFXVolume;
		}
	}
	else
	{
		MasterVolume = 1.0f;
		BGMVolume = 1.0f;
		SFXVolume = 1.0f;
	}

	EnsureSoundClassVolumesApplied();
}
