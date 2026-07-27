#include "QPLobbyPlayerController.h"
#include "Blueprint/UserWidget.h"

void AQPLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && LobbyWidgetClass)
	{
		LobbyWidgetInstance = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
			
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
		}
	}
}
