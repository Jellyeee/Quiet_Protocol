#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ZombieStartAttack.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UBTTask_ZombieStartAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZombieStartAttack();
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector isAttackingKey; // 공격중인지 여부 확인

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};