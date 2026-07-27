// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "QPGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	// 캐릭터 사망 시 10초 대기 후 리스폰을 요청하는 함수
	UFUNCTION(BlueprintCallable, Category = "GameMode")
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController);

protected:
	virtual void BeginPlay() override;

	/** 해당 게임모드 기본 BGM 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QP|Sound")
	TObjectPtr<class USoundBase> GameBGM;
};
