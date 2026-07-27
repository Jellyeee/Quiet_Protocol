#include "QPServerListEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UQPServerListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!JoinButton) JoinButton = Cast<UButton>(GetWidgetFromName(TEXT("JoinButton")));
	if (!HostNameText) HostNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("HostNameText")));
	if (!PingText) PingText = Cast<UTextBlock>(GetWidgetFromName(TEXT("PingText")));
	if (!PlayerCountText) PlayerCountText = Cast<UTextBlock>(GetWidgetFromName(TEXT("PlayerCountText")));

	if (JoinButton)
	{
		JoinButton->OnClicked.AddUniqueDynamic(this, &UQPServerListEntryWidget::OnJoinButtonClicked);
	}
}

void UQPServerListEntryWidget::Setup(UQPSessionSubsystem* InSubsystem, const FQPBlueprintSessionInfo& SessionInfo)
{
	SessionSubsystem = InSubsystem;
	SessionIndex = SessionInfo.SessionIndex;

	if (HostNameText)
	{
		HostNameText->SetText(FText::FromString(SessionInfo.HostName));
	}

	if (PingText)
	{
		PingText->SetText(FText::FromString(FString::Printf(TEXT("%d ms"), SessionInfo.Ping)));
	}

	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), SessionInfo.CurrentPlayers, SessionInfo.MaxPlayers)));
	}
}

void UQPServerListEntryWidget::OnJoinButtonClicked()
{
	if (SessionSubsystem)
	{
		SessionSubsystem->JoinSessionAtIndex(SessionIndex);
		
		if (JoinButton)
		{
			JoinButton->SetIsEnabled(false);
		}
	}
}
