#include "QPLobbyWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "PJ_Quiet_Protocol/Session/QPSessionSubsystem.h"

void UQPLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &UQPLobbyWidget::OnStartGameClicked);
		
		// 호스트(서버)가 아니면 시작 버튼을 화면에서 숨기고, 서버라면 명시적으로 보여줍니다.
		if (GetOwningPlayer() && !GetOwningPlayer()->HasAuthority())
		{
			StartGameButton->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			StartGameButton->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (QuitLobbyButton)
	{
		QuitLobbyButton->OnClicked.AddDynamic(this, &UQPLobbyWidget::OnQuitLobbyClicked);
	}

	// 1초 간격으로 플레이어 목록 갱신 시작
	GetWorld()->GetTimerManager().SetTimer(UpdatePlayerListTimerHandle, this, &UQPLobbyWidget::UpdatePlayerList, 1.0f, true);
	UpdatePlayerList(); // 즉시 한 번 갱신
}

void UQPLobbyWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// 위젯 파괴 시 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdatePlayerListTimerHandle);
	}
}

void UQPLobbyWidget::OnStartGameClicked()
{
	if (StartGameButton)
	{
		StartGameButton->SetIsEnabled(false);
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = GetWorld()->GetFirstPlayerController();
	}

	if (PC)
	{
		if (PC->HasAuthority())
		{
			UKismetSystemLibrary::PrintString(this, TEXT("ServerTravel 시도 중..."), true, true, FLinearColor::Green, 5.0f);
			GetWorld()->ServerTravel("/Game/Maps/InGame?listen");
		}
		else
		{
			UKismetSystemLibrary::PrintString(this, TEXT("권한(Authority)이 없습니다. 클라이언트로 인식됨!"), true, true, FLinearColor::Red, 5.0f);
			if (StartGameButton) StartGameButton->SetIsEnabled(true);
		}
	}
	else
	{
		UKismetSystemLibrary::PrintString(this, TEXT("PlayerController를 찾을 수 없습니다!"), true, true, FLinearColor::Red, 5.0f);
		if (StartGameButton) StartGameButton->SetIsEnabled(true);
	}
}

void UQPLobbyWidget::OnQuitLobbyClicked()
{
	if (QuitLobbyButton)
	{
		QuitLobbyButton->SetIsEnabled(false);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UQPSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UQPSessionSubsystem>())
		{
			// 멀티플레이 세션 파괴 이벤트 구독
			SessionSubsystem->OnDestroySessionCompleteEvent.AddDynamic(this, &UQPLobbyWidget::OnDestroySessionComplete);
			
			// 세션 파괴 호출
			SessionSubsystem->DestroySession();
			return; 
		}
	}

	// 실패 시 강제 이동
	OnDestroySessionComplete(false);
}

void UQPLobbyWidget::OnDestroySessionComplete(bool bWasSuccessful)
{
	// 메인 메뉴 맵으로 강제 이동
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel("/Game/Maps/Main", ETravelType::TRAVEL_Absolute);
	}
	else
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName("Main"));
	}
}

void UQPLobbyWidget::UpdatePlayerList()
{
	if (!PlayerListContainer) return;

	UWorld* World = GetWorld();
	if (!World) return;

	AGameStateBase* GameState = World->GetGameState<AGameStateBase>();
	if (!GameState) return;

	// 기존 목록 초기화
	PlayerListContainer->ClearChildren();

	// 현재 접속해 있는 모든 PlayerState 순회
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS)
		{
			// 텍스트 블록 생성 및 설정
			UTextBlock* PlayerNameText = NewObject<UTextBlock>(this);
			PlayerNameText->SetText(FText::FromString(PS->GetPlayerName()));
			
			// 글씨 크기나 여백 설정 (기본적으로 24사이즈 설정)
			FSlateFontInfo FontInfo = PlayerNameText->GetFont();
			FontInfo.Size = 24;
			PlayerNameText->SetFont(FontInfo);

			// 컨테이너에 추가
			PlayerListContainer->AddChild(PlayerNameText);
		}
	}
}
