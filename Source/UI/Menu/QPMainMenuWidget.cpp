#include "PJ_Quiet_Protocol/UI/Menu/QPMainMenuWidget.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/Session/QPSessionSubsystem.h"

void UQPMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ServerListContainer) ServerListContainer = Cast<UWidget>(GetWidgetFromName(TEXT("ServerListContainer")));
	if (!ServerListScrollBox) ServerListScrollBox = Cast<UScrollBox>(GetWidgetFromName(TEXT("ServerListScrollBox")));
	if (!RefreshButton) RefreshButton = Cast<UButton>(GetWidgetFromName(TEXT("RefreshButton")));
	if (!Host) Host = Cast<UButton>(GetWidgetFromName(TEXT("Host")));
	if (!Join) Join = Cast<UButton>(GetWidgetFromName(TEXT("Join")));
	if (!Option) Option = Cast<UButton>(GetWidgetFromName(TEXT("Option")));
	if (!Quit) Quit = Cast<UButton>(GetWidgetFromName(TEXT("Quit")));

	if (Host)
	{
		Host->OnClicked.AddUniqueDynamic(this, &UQPMainMenuWidget::OnHostButtonClicked);
	}

	if (Join)
	{
		Join->OnClicked.AddUniqueDynamic(this, &UQPMainMenuWidget::OnJoinButtonClicked);
	}

	if (Option)
	{
		Option->OnClicked.AddUniqueDynamic(this, &UQPMainMenuWidget::OnOptionsButtonClicked);
	}

	if (Quit)
	{
		Quit->OnClicked.AddUniqueDynamic(this, &UQPMainMenuWidget::OnQuitButtonClicked);
	}

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddUniqueDynamic(this, &UQPMainMenuWidget::OnRefreshButtonClicked);
	}
	
	if (ServerListContainer)
	{
		ServerListContainer->SetVisibility(ESlateVisibility::Hidden); // 처음에는 숨김
		UE_LOG(LogTemp, Warning, TEXT("[UI] ServerListContainer is successfully bound!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UI] ServerListContainer failed to bind (it is NULL)"));
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UQPSessionSubsystem* Subsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
		{
			Subsystem->OnCreateSessionCompleteEvent.AddDynamic(this, &UQPMainMenuWidget::OnCreateSessionComplete);
			Subsystem->OnFindSessionsCompleteEvent.AddDynamic(this, &UQPMainMenuWidget::OnFindSessionsComplete);
			Subsystem->OnJoinSessionCompleteEvent.AddDynamic(this, &UQPMainMenuWidget::OnJoinSessionComplete);
		}
	}
}

void UQPMainMenuWidget::PlayClickSound()
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

void UQPMainMenuWidget::OnHostButtonClicked()
{
	PlayClickSound();

	if (Host) Host->SetIsEnabled(false); // 더블 클릭 방지
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("[UI] Host Button Clicked!"));

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UQPSessionSubsystem* Subsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
		{
			Subsystem->CreateSession(4, false); // 4인방, 스팀 기반
		}
	}
}

void UQPMainMenuWidget::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, bWasSuccessful ? FColor::Green : FColor::Red, FString::Printf(TEXT("[UI] OnCreateSessionComplete: %d"), bWasSuccessful));

	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel("/Game/Maps/Lobby?listen");
		}
	}
	else
	{
		if (Host) Host->SetIsEnabled(true);
	}
}

void UQPMainMenuWidget::OnJoinButtonClicked()
{
	PlayClickSound();

	if (ServerListContainer)
	{
		ServerListContainer->SetVisibility(ESlateVisibility::Visible);
		ServerListContainer->SetRenderOpacity(1.0f);
		
		// InnerBox 강제 활성화
		if (UPanelWidget* BorderPanel = Cast<UPanelWidget>(ServerListContainer))
		{
			if (BorderPanel->GetChildrenCount() > 0)
			{
				if (UWidget* InnerBoxWidget = BorderPanel->GetChildAt(0))
				{
					InnerBoxWidget->SetVisibility(ESlateVisibility::Visible);
					InnerBoxWidget->SetRenderOpacity(1.0f);
				}
			}
		}
		
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("[UI] ServerListContainer is VALID and set to VISIBLE"));
	}

	if (RefreshButton)
	{
		RefreshButton->SetVisibility(ESlateVisibility::Visible); // 강제로 보이게 켜봅니다.
		RefreshButton->SetRenderOpacity(1.0f);
	}

	if (ServerListScrollBox)
	{
		ServerListScrollBox->SetVisibility(ESlateVisibility::Visible);
		ServerListScrollBox->SetRenderOpacity(1.0f);
		ServerListScrollBox->ClearChildren(); // 검색할 때마다 기존 목록 초기화 (추가)
	}

	OnRefreshButtonClicked();
}

void UQPMainMenuWidget::OnRefreshButtonClicked()
{
	PlayClickSound();

	if (RefreshButton)
	{
		RefreshButton->SetIsEnabled(false); // 검색 중 버튼 비활성화
	}

	if (Join)
	{
		Join->SetIsEnabled(false); // Join 버튼도 잠시 비활성화
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UQPSessionSubsystem* Subsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
		{
			Subsystem->FindSessions(10000, false);
		}
	}
}

void UQPMainMenuWidget::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (RefreshButton)
	{
		RefreshButton->SetIsEnabled(true);
	}
	
	if (Join)
	{
		Join->SetIsEnabled(true);
	}

	if (ServerListScrollBox)
	{
		ServerListScrollBox->ClearChildren();
	}

	if (bWasSuccessful)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UQPSessionSubsystem* Subsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
			{
				TArray<FQPBlueprintSessionInfo> FoundSessions = Subsystem->GetFoundSessions();
				
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("[UI] FoundSessions Num: %d"), FoundSessions.Num()));
				
				if (ServerListEntryClass == nullptr)
				{
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[UI] ERROR: ServerListEntryClass is NULL! Please assign WBP_ServerListEntry in Blueprint Details!"));
				}
				if (ServerListScrollBox == nullptr)
				{
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[UI] ERROR: ServerListScrollBox is NULL! It failed to find the widget."));
				}

				if (FoundSessions.Num() > 0 && ServerListEntryClass != nullptr && ServerListScrollBox != nullptr)
				{
					for (const FQPBlueprintSessionInfo& SessionInfo : FoundSessions)
					{
						UQPServerListEntryWidget* EntryWidget = CreateWidget<UQPServerListEntryWidget>(this, ServerListEntryClass);
						if (EntryWidget)
						{
							EntryWidget->Setup(Subsystem, SessionInfo);
							ServerListScrollBox->AddChild(EntryWidget);
							
							// 크기가 0으로 찌그러졌는지 확인하는 강력한 로그
							FVector2D ScrollSize = ServerListScrollBox->GetCachedGeometry().GetLocalSize();
							if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Emerald, FString::Printf(TEXT("[UI] ScrollBox Size: %f x %f"), ScrollSize.X, ScrollSize.Y));
							if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("[UI] Added Session Entry to ScrollBox!"));
						}
					}
				}
			}
		}
	}
}

void UQPMainMenuWidget::OnJoinSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		if (Join) Join->SetIsEnabled(true); // 실패 시 버튼 복구
	}
}

void UQPMainMenuWidget::OnOptionsButtonClicked()
{
	PlayClickSound();

	ShowOptionsWidget();

	if (OptionsWidgetClass)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			UUserWidget* OptionsWidget = CreateWidget<UUserWidget>(PC, OptionsWidgetClass);
			if (OptionsWidget)
			{
				OptionsWidget->AddToViewport(10);
			}
		}
	}
}

void UQPMainMenuWidget::OnQuitButtonClicked()
{
	PlayClickSound();

	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
