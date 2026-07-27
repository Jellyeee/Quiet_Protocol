#include "PJ_Quiet_Protocol/Commons/QPSettingsSaveGame.h"

UQPSettingsSaveGame::UQPSettingsSaveGame()
{
	MasterVolume = 1.0f;
	BGMVolume = 1.0f;
	SFXVolume = 1.0f;
	MouseSensitivity = 1.0f;
	ResolutionQuality = 100;
	
	SaveSlotName = TEXT("QPSettingsSaveSlot");
	UserIndex = 0;
}
