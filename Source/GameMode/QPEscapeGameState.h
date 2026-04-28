#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "QPEscapeGameState.generated.h"

/**
 * 
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPEscapeGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AQPEscapeGameState();

	// 남은 발전기 개수
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	int32 RemainingGenerators;

	// 전체 발전기 개수 (비밀번호 길이)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	int32 TotalGenerators;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
