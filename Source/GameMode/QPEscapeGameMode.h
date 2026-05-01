#pragma once

#include "CoreMinimal.h"
#include "PJ_Quiet_Protocol/GameMode/QPGameMode.h"
#include "QPEscapeGameMode.generated.h"

class AQPEscapeDoor;
class AQPEscapeGenerator;

/**
 * AQPEscapeGameMode
 * 
 * 탈출 게임의 규칙과 마스터 패스워드 로직을 관리합니다.
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPEscapeGameMode : public AQPGameMode
{
	GENERATED_BODY()
	
public:
	AQPEscapeGameMode();

	// 탈출 시도 검증 (제출한 패스워드 배열과 시도한 문을 기준으로 검증)
	UFUNCTION(BlueprintCallable, Category = "Escape")
	bool VerifyEscapeAttempt(const TArray<int32>& InputPassword, AQPEscapeDoor* TargetDoor);

	// 플레이어가 출구를 통해 빠져나갔을 때의 처리 (관전 모드 전환 등)
	void OnPlayerEscaped(class AQPCharacter* EscapedPlayer);

	// 마스터 패스워드에 자릿수 추가 및 등록 (발전기 쪽에서 BeginPlay 시점에 호출)
	UFUNCTION(BlueprintCallable, Category = "Escape")
	void RegisterGenerator(class AQPEscapeGenerator* Generator);

protected:
	virtual void BeginPlay() override;

	// 랜덤으로 진짜 출구 지정
	void SetRealExits();

private:
	// 생성된 정답 패스워드 (발전기가 등록될 때마다 자릿수가 늘어남)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape", meta = (AllowPrivateAccess = "true"))
	TArray<int32> MasterPassword;

	// 맵에 존재하는 모든 출구 배열
	UPROPERTY()
	TArray<AQPEscapeDoor*> AllDoors;

	// 맵에 존재하는 모든 발전기 배열
	UPROPERTY()
	TArray<AQPEscapeGenerator*> AllGenerators;
};
