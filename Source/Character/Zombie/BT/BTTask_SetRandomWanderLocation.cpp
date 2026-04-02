#include "BTTask_SetRandomWanderLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
UBTTask_SetRandomWanderLocation::UBTTask_SetRandomWanderLocation()
{
	NodeName = TEXT("Set Random Wander Location");
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetRandomWanderLocation, BlackboardKey));
}

EBTNodeResult::Type UBTTask_SetRandomWanderLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	/** AI 컨트롤러 및 제어 중인 전당(Pawn) 유효성 검사 */
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Pawn || !BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}
	const FVector Origin = Pawn->GetActorLocation();
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	/** 
	 * [랜덤 포인트 탐색 로직]
	 * 단순히 주변의 랜덤한 점을 찍는 것이 아니라, 
	 * 너무 가까운 위치가 잡히지 않도록 최대 8번까지 재시도합니다.
	 */
	const int32 MaxTries = 8;
	FNavLocation OutLocation;
	bool bFound = false;

	for (int32 i = 0; i < MaxTries; ++i)
	{
		// 내비게이션 메쉬 범위 내에서 WanderRadius 반경 안의 랜덤한 지점을 찾습니다.
		if (NavSys->GetRandomPointInNavigableRadius(Origin, WanderRadius, OutLocation))
		{
			// 현재 위치와 찾은 지점 사이의 수평 거리(2D)를 계산합니다.
			const float Distance2D = FVector::Dist2D(Origin, OutLocation.Location);
			
			// 설정된 최소 거리(MinDistanceFromOrigin)보다 멀리 떨어진 곳일 때만 유효한 지점으로 간주합니다.
			if (Distance2D >= MinDistanceFromOrigin)
			{
				bFound = true;
				break;
			}
		}
	}

	if (!bFound) return EBTNodeResult::Failed;

	/** 찾은 좌표를 블랙보드 키에 저장하여 이후 MoveTo 태스크에서 사용할 수 있도록 합니다. */
	BlackboardComponent->SetValueAsVector(BlackboardKey.SelectedKeyName, OutLocation.Location);
	return EBTNodeResult::Succeeded;
}