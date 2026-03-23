#include "ZombieAIController.h"
#include "BehaviorTree/BehaviorTree.h"

AZombieAIController::AZombieAIController()
{
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn); //부모 클래스의 OnPossess 호출
	if (BehaviorTreeAsset) {
		RunBehaviorTree(BehaviorTreeAsset); //행동 트리 실행
	}
}