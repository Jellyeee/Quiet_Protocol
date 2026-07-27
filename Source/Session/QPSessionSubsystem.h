#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "QPSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQPOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQPOnFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQPOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQPOnDestroySessionComplete, bool, bWasSuccessful);

USTRUCT(BlueprintType)
struct FQPBlueprintSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 SessionIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 Ping = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;
};

/**
 * 멀티플레이어 세션을 관리하는 커스텀 게임 인스턴스 서브시스템
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UQPSessionSubsystem();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(int32 NumPublicConnections, bool bIsLAN);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(int32 MaxSearchResults, bool bIsLAN);

	UFUNCTION(BlueprintCallable, Category = "Session")
	TArray<FQPBlueprintSessionInfo> GetFoundSessions() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSessionAtIndex(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSession(); 

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UPROPERTY(BlueprintAssignable)
	FQPOnCreateSessionComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable)
	FQPOnFindSessionsComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable)
	FQPOnJoinSessionComplete OnJoinSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable)
	FQPOnDestroySessionComplete OnDestroySessionCompleteEvent;

protected:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	int32 PendingNumPublicConnections;
	bool bPendingIsLAN;
	bool bIsCreatingSession;

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<class FOnlineSessionSearch> LastSessionSearch;
};
