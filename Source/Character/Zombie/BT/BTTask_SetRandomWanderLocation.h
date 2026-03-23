#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_SetRandomWanderLocation.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UBTTask_SetRandomWanderLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UBTTask_SetRandomWanderLocation(); //생성자 선언
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override; //ExecuteTask 함수 재정의
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderRadius = 900.f; //배회 반경
	UPROPERTY(EditAnywhere, Category = "Wander")
	float MinDistanceFromOrigin = 300.f; //원점으로부터의 최소 거리

};