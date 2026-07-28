#include "QPSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"

UQPSessionSubsystem::UQPSessionSubsystem()
{
}

void UQPSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLAN)
{
	UE_LOG(LogTemp, Warning, TEXT("[Session] CreateSession Called!"));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("[Session] CreateSession Called!"));

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Session] SessionInterface is Invalid!"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[Session] SessionInterface is Invalid!"));
		OnCreateSessionCompleteEvent.Broadcast(false);
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] Existing session found, destroying..."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[Session] Destroying existing session... please wait."));
		
		PendingNumPublicConnections = NumPublicConnections;
		bPendingIsLAN = bIsLAN;
		
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UQPSessionSubsystem::OnDestroySessionComplete));
		SessionInterface->DestroySession(NAME_GameSession);
		return; // 파괴가 완료되면 OnDestroySessionComplete에서 다시 CreateSession을 부릅니다.
	}

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UQPSessionSubsystem::OnCreateSessionComplete));

	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = bIsLAN;
	SessionSettings->NumPublicConnections = NumPublicConnections;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true; // 스팀 P2P 통신(리슨 서버)을 위해 반드시 true여야 합니다.
	SessionSettings->bUseLobbiesIfAvailable = true; // 언리얼5 스팀 연동에서는 bUsesPresence가 true면 반드시 이것도 true여야 CreateSession이 실패하지 않습니다.
	SessionSettings->Set(FName("MatchType"), FString("QuietProtocol_Jelly_Test_001"), EOnlineDataAdvertisementType::ViaOnlineService);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("[Session] LocalPlayer is NULL!"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[Session] LocalPlayer is NULL!"));
	}

	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings))
	{
		UE_LOG(LogTemp, Error, TEXT("[Session] Failed to start CreateSession! (Returns False)"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[Session] Failed to start CreateSession! (Returns False)"));
		SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
		OnCreateSessionCompleteEvent.Broadcast(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] CreateSession Successfully Started!"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("[Session] CreateSession Successfully Started!"));
	}
}

void UQPSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[Session] OnCreateSessionComplete: %d"), bWasSuccessful);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, bWasSuccessful ? FColor::Green : FColor::Red, FString::Printf(TEXT("[Session] OnCreateSessionComplete: %d"), bWasSuccessful));

	SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
	OnCreateSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void UQPSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, bWasSuccessful ? FColor::Green : FColor::Red, FString::Printf(TEXT("[Session] DestroySessionComplete: %d"), bWasSuccessful));
	SessionInterface->ClearOnDestroySessionCompleteDelegates(this);

	if (bWasSuccessful && PendingNumPublicConnections > 0)
	{
		// 파괴가 완료되었으므로 이제 다시 생성을 시도합니다.
		CreateSession(PendingNumPublicConnections, bPendingIsLAN);
	}
	else
	{
		OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);
	}
}

void UQPSessionSubsystem::DestroySession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	if (!SessionInterface.IsValid())
	{
		OnDestroySessionCompleteEvent.Broadcast(false);
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		PendingNumPublicConnections = 0; // 재성성 방지
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UQPSessionSubsystem::OnDestroySessionComplete));
		SessionInterface->DestroySession(NAME_GameSession);
	}
	else
	{
		OnDestroySessionCompleteEvent.Broadcast(true); // 없으면 성공 처리
	}
}

void UQPSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLAN)
{
	UE_LOG(LogTemp, Warning, TEXT("[Session] FindSessions Started - LAN: %d, MaxResults: %d"), bIsLAN, MaxSearchResults);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("[Session] FindSessions Started - LAN: %d, MaxResults: %d"), bIsLAN, MaxSearchResults));

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	if (!SessionInterface.IsValid())
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UQPSessionSubsystem::OnFindSessionsComplete));

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = bIsLAN;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals); // bUsesPresence=true 이므로 검색도 true
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals); // 스팀 로비 접속을 위해 필수!
	LastSessionSearch->QuerySettings.Set(FName("MatchType"), FString("QuietProtocol_Jelly_Test_001"), EOnlineComparisonOp::Equals);

	if (!SessionInterface->FindSessions(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Error, TEXT("[Session] FindSessions Failed to start search"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[Session] FindSessions Failed to start search"));
		SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
		OnFindSessionsCompleteEvent.Broadcast(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] FindSessions successfully started searching"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("[Session] FindSessions successfully started searching"));
	}
}

void UQPSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[Session] OnFindSessionsComplete - bWasSuccessful: %d"), bWasSuccessful);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, bWasSuccessful ? FColor::Green : FColor::Red, FString::Printf(TEXT("[Session] OnFindSessionsComplete - bWasSuccessful: %d"), bWasSuccessful));

	if (bWasSuccessful && LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] Found %d sessions."), LastSessionSearch->SearchResults.Num());
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("[Session] Found %d sessions."), LastSessionSearch->SearchResults.Num()));
	}

	SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
	OnFindSessionsCompleteEvent.Broadcast(bWasSuccessful);
}

void UQPSessionSubsystem::JoinSession()
{
	UE_LOG(LogTemp, Warning, TEXT("[Session] JoinSession Called"));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("[Session] JoinSession Called"));

	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Session] SessionInterface or LastSessionSearch is invalid"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[Session] SessionInterface or LastSessionSearch is invalid"));
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	JoinSessionAtIndex(0);
}

TArray<FQPBlueprintSessionInfo> UQPSessionSubsystem::GetFoundSessions() const
{
	TArray<FQPBlueprintSessionInfo> SessionInfos;

	if (LastSessionSearch.IsValid())
	{
		for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[i];
			if (SearchResult.IsValid())
			{
				FQPBlueprintSessionInfo Info;
				Info.SessionIndex = i;
				Info.HostName = SearchResult.Session.OwningUserName;
				Info.Ping = SearchResult.PingInMs;
				Info.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
				Info.CurrentPlayers = Info.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
				
				SessionInfos.Add(Info);
			}
		}
	}

	return SessionInfos;
}

void UQPSessionSubsystem::JoinSessionAtIndex(int32 SessionIndex)
{
	if (!SessionInterface.IsValid()) return;
	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= SessionIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] Invalid SessionIndex: %d"), SessionIndex);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] Existing session found. Destroying before Join..."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, TEXT("[Session] Cleaning up old session... Please click '참가' again in 2 seconds!"));
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[SessionIndex];

	FString HostName = SearchResult.Session.OwningUserName;
	UE_LOG(LogTemp, Warning, TEXT("[Session] Attempting to join session index %d... (Host: %s, Ping: %d)"), SessionIndex, *HostName, SearchResult.PingInMs);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("[Session] Attempting to join... Host: %s, Ping: %d"), *HostName, SearchResult.PingInMs));

	// [DIAGNOSTIC LOGS]
	bool bSessionInfoValid = SearchResult.Session.SessionInfo.IsValid();
	FString SessionIdStr = bSessionInfoValid ? SearchResult.Session.SessionInfo->GetSessionId().ToString() : TEXT("INVALID");
	bool bSessionIdValid = bSessionInfoValid ? SearchResult.Session.SessionInfo->GetSessionId().IsValid() : false;
	FString SessionIdType = bSessionInfoValid ? SearchResult.Session.SessionInfo->GetSessionId().GetType().ToString() : TEXT("NONE");
	bool bUsesPresence = SearchResult.Session.SessionSettings.bUsesPresence;
	bool bUseLobbies = SearchResult.Session.SessionSettings.bUseLobbiesIfAvailable;
	bool bIsLAN = SearchResult.Session.SessionSettings.bIsLANMatch;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString::Printf(TEXT("[Diag] InfoValid:%d, IdValid:%d"), bSessionInfoValid, bSessionIdValid));
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString::Printf(TEXT("[Diag] SessionId: %s"), *SessionIdStr));
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString::Printf(TEXT("[Diag] SessionType: %s"), *SessionIdType));
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString::Printf(TEXT("[Diag] Presence:%d, Lobbies:%d, LAN:%d"), bUsesPresence, bUseLobbies, bIsLAN));
	}

	// [UE 5.5 스팀 버그 우회 조치]
	// UE 5.5부터 FOnlineSessionSettings의 bUsesPresence와 bUseLobbiesIfAvailable이 일치하지 않으면 (예: 1과 0) 
	// 내부 검증에서 JoinSession이 즉시 UnknownError (5)를 뱉으며 종료되는 버그가 있습니다.
	// 검색 결과 구조체를 강제 수정 가능하도록 캐스팅하여 두 값을 모두 true로 일치시켜 줍니다.
	FOnlineSessionSearchResult& ModifiableResult = const_cast<FOnlineSessionSearchResult&>(SearchResult);
	ModifiableResult.Session.SessionSettings.bUsesPresence = true;
	ModifiableResult.Session.SessionSettings.bUseLobbiesIfAvailable = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, TEXT("[Diag-BugFix] Mismatched Steam Flags forced to MATCH (True)!"));
	}

	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UQPSessionSubsystem::OnJoinSessionComplete));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer && LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		FUniqueNetIdRepl NetIdRepl = LocalPlayer->GetPreferredUniqueNetId();
		FName NetIdType = NetIdRepl.GetUniqueNetId() ? NetIdRepl.GetUniqueNetId()->GetType() : NAME_None;
		
		UE_LOG(LogTemp, Warning, TEXT("[Session] NetId Valid! Type: %s"), *NetIdType.ToString());
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("[Session] NetId Type: %s"), *NetIdType.ToString()));
		
		if (NetIdType != FName("Steam"))
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("[Error] NetId is NOT Steam! (Editor PIE issue?)"));
		}

		if (!SessionInterface->JoinSession(*NetIdRepl, NAME_GameSession, ModifiableResult))
		{
			UE_LOG(LogTemp, Error, TEXT("[Session] Failed to start JoinSession (Returned False)"));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("[Session] Failed to start JoinSession (Returns False)"));
			SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
			OnJoinSessionCompleteEvent.Broadcast(false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] LocalPlayer or NetId is invalid, falling back to LocalUserNum 0"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, TEXT("[Session] Using PlayerNum 0 Fallback"));
		
		if (!SessionInterface->JoinSession(0, NAME_GameSession, ModifiableResult))
		{
			UE_LOG(LogTemp, Error, TEXT("[Session] Failed to start JoinSession (Returned False)"));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("[Session] Failed to start JoinSession with index 0"));
			SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
			OnJoinSessionCompleteEvent.Broadcast(false);
		}
	}
}

void UQPSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	FString ResultString;
	switch (Result)
	{
		case EOnJoinSessionCompleteResult::Success: ResultString = TEXT("Success (0) - 성공!"); break;
		case EOnJoinSessionCompleteResult::SessionIsFull: ResultString = TEXT("SessionIsFull (1) - 방이 꽉 찼습니다."); break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: ResultString = TEXT("SessionDoesNotExist (2) - 방이 존재하지 않습니다."); break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: ResultString = TEXT("CouldNotRetrieveAddress (3) - IP/스팀 주소를 가져올 수 없습니다."); break;
		case EOnJoinSessionCompleteResult::AlreadyInSession: ResultString = TEXT("AlreadyInSession (4) - 이미 방에 접속되어 있습니다."); break;
		case EOnJoinSessionCompleteResult::UnknownError: ResultString = TEXT("UnknownError (5) - 알 수 없는 에러 (스팀 네트워크 또는 로비 연결 거부)"); break;
		default: ResultString = FString::Printf(TEXT("Other Error Code: %d"), (int32)Result); break;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Session] OnJoinSessionComplete - Result: %s"), *ResultString);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.f, (Result == EOnJoinSessionCompleteResult::Success) ? FColor::Green : FColor::Red, FString::Printf(TEXT("[Join Result] %s"), *ResultString));

	SessionInterface->ClearOnJoinSessionCompleteDelegates(this);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Session] ConnectString Resolved: %s"), *ConnectString);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString::Printf(TEXT("[Session] ConnectString: %s"), *ConnectString));

			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Session] Failed to Resolve Connect String (Host's Steam ID missing!)"));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("[Error] Failed to Resolve Connect String! 호스트의 스팀 주소를 획득하지 못했습니다. (bInitServerOnClient=true 확인 필요)"));
		}
		OnJoinSessionCompleteEvent.Broadcast(true);
	}
	else
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
	}
}
