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

	// 캐릭터 사망 시 리스폰 요청 처리 (탈출 성공 후에는 리스폰 차단 목적)
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController) override;

	// 마스터 패스워드에 자릿수 추가 및 등록 (발전기 쪽에서 BeginPlay 시점에 호출)
	UFUNCTION(BlueprintCallable, Category = "Escape")
	void RegisterGenerator(class AQPEscapeGenerator* Generator);

protected:
	virtual void BeginPlay() override;

	/** 인게임 BGM 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Sound")
	TObjectPtr<class USoundBase> InGameBGM;

	// 랜덤으로 진짜 출구 지정
	void SetRealExits();

	// 라운드 종료 연출을 위한 NetMulticast RPC
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnGameFinished(bool bVictory, const FString& WinnerNames);

private:
	// 스폰할 발전기 블루프린트 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AQPEscapeGenerator> GeneratorClass;

	// 생성할 발전기 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Generator", meta = (AllowPrivateAccess = "true"))
	int32 NumGeneratorsToSpawn = 4;

	// 생성된 정답 패스워드 (발전기가 등록될 때마다 자릿수가 늘어남)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape", meta = (AllowPrivateAccess = "true"))
	TArray<int32> MasterPassword;

	// 맵에 존재하는 모든 출구 배열
	UPROPERTY()
	TArray<AQPEscapeDoor*> AllDoors;

	// 맵에 존재하는 모든 발전기 배열
	UPROPERTY()
	TArray<AQPEscapeGenerator*> AllGenerators;

	// 탈출 성공한 플레이어 목록
	UPROPERTY()
	TArray<APlayerController*> EscapedPlayers;

	// 게임 종료 상태 여부
	bool bGameFinished;
};
