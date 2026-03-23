// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "Zombie_BTService_UpdateTarget.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UZombie_BTService_UpdateTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UZombie_BTService_UpdateTarget(); //생성자 선언

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override; //TickNode 함수 재정의

	UPROPERTY(EditAnywhere, Category = "Target")
	float DetectRange = 2000.f; //탐지 범위

	UPROPERTY(EditAnywhere, Category = "Target")
	float LoseTargetRange = 3000.f; //타겟 상실 범위

	UPROPERTY(EditAnywhere, Category = "Target")
	bool bRequireLineOfSight = true; //시야 요구 여부

	UPROPERTY(EditAnywhere, Category = "Target")
	bool bFindClosestPlayer = true; //가장 가까운 플레이어 찾기 여부

	UPROPERTY(EditAnywhere, Category = "Target|View")
	float DetectFovDegress = 90.f; //탐지 시야각
	UPROPERTY(EditAnywhere, Category = "Target|View")
	float LoseFOVDegrees = 180.f; //타겟 상실 시야각
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LastKnownLocationKey; //블랙보드 키 선택기
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector InvestigatingKey; //블랙보드 키 선택기

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasTargetKey; //블랙보드 키 선택기

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetKey; //블랙보드 키 선택기

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector IsAttackingKey; //블랙보드 키 선택기
};