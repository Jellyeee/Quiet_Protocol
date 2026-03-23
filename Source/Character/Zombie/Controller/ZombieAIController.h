// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZombieAIController.generated.h"

class UBehaviorTree;

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AZombieAIController : public AAIController
{
	GENERATED_BODY()
public:
	AZombieAIController();
protected:
	virtual void OnPossess(APawn* InPawn) override;
public:
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	UBehaviorTree* BehaviorTreeAsset = nullptr; //행동 트리 에셋
};