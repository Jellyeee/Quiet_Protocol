// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearInvestigating.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UBTTask_ClearInvestigating : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearInvestigating(); //생성자 선언

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector InvestigatingKey; //조사 중인 위치를 나타내는 블랙보드 키 선택기
protected:
	virtual EBTNodeResult::Type	ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};