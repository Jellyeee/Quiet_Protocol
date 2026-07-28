#include "PJ_Quiet_Protocol/UI/Menu/QPOptionsWidget.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "GameFramework/GameUserSettings.h"

void UQPOptionsWidget::PlayClickSound()
{
	if (ClickSound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlayUISound(ClickSound);
			}
		}
	}
}

void UQPOptionsWidget::ApplySettings()
{
	PlayClickSound();

	// 1. 그래픽 설정 적용
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->ApplySettings(false);
	}

	// 2. 오디오 볼륨 설정 저장
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			AudioSubsystem->SaveAudioSettings();
		}
	}

	OnSettingsApplied();
}

void UQPOptionsWidget::CloseOptions()
{
	PlayClickSound();

	OnOptionsClosed();
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
}

void UQPOptionsWidget::SetMasterVolume(float Volume)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			AudioSubsystem->SetMasterVolume(Volume);
		}
	}
}

void UQPOptionsWidget::SetBGMVolume(float Volume)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			AudioSubsystem->SetBGMVolume(Volume);
		}
	}
}

void UQPOptionsWidget::SetSFXVolume(float Volume)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			AudioSubsystem->SetSFXVolume(Volume);
		}
	}
}

float UQPOptionsWidget::GetMasterVolume() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			return AudioSubsystem->GetMasterVolume();
		}
	}
	return 1.0f;
}

float UQPOptionsWidget::GetBGMVolume() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			return AudioSubsystem->GetBGMVolume();
		}
	}
	return 1.0f;
}

float UQPOptionsWidget::GetSFXVolume() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			return AudioSubsystem->GetSFXVolume();
		}
	}
	return 1.0f;
}
