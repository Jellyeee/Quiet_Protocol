#include "BTTask_SetRandomWanderLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
UBTTask_SetRandomWanderLocation::UBTTask_SetRandomWanderLocation()
{
	NodeName = TEXT("Set Random Wander Location"); //노드 이름 설정
	// BT에서 Vector 키로 지정할 거라서 타입 제한을 걸어둠
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetRandomWanderLocation, BlackboardKey)); //블랙보드 키에 Vector 타입 필터 추가)

}

EBTNodeResult::Type UBTTask_SetRandomWanderLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory); //부모 클래스의 ExecuteTask 호출
	AAIController* AIController = OwnerComp.GetAIOwner(); //AI 컨트롤러 가져오기
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr; //AI 컨트롤러의 폰 가져오기
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent(); //블랙보드 컴포넌트 가져오기
	if (!AIController || !Pawn || !BlackboardComponent)
	{
		return EBTNodeResult::Failed; //유효성 검사 실패 시 실패 반환
	}
	const FVector Origin = Pawn->GetActorLocation(); //현재 폰의 위치를 원점으로 설정
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld()); //네비게이션 시스템 가져오기
	if (!NavSys) return EBTNodeResult::Failed; //네비게이션 시스템 유효성 검사 실패 시 실패 반환
	//너무 가까운 포인트가 계속되면 제자리 걸음이 될 수 있으므로 시도 횟수 제한
	const int32 MaxTries = 8; //최대 시도 횟수
	FNavLocation OutLocation; //네비게이션 위치 변수
	bool bFound = false; //위치 발견 여부 변수

	for (int32 i = 0; i < MaxTries; ++i) //최대 시도 횟수만큼 반복
	{
		//네비게이션 시스템을 사용해 원점 주변의 랜덤 위치 찾기
		if (NavSys->GetRandomPointInNavigableRadius(Origin, WanderRadius, OutLocation))
		{
			//찾은 위치가 원점에서 최소 거리 이상 떨어져 있는지 확인
			const float Distance2D = FVector::Dist2D(Origin, OutLocation.Location); //원점으로부터의 거리 계산
			if (Distance2D >= MinDistanceFromOrigin) //최소 거리 조건 만족 시
			{
				bFound = true; //위치 발견 여부 설정
				break; //반복문 종료
			}
		}
	}

	if (!bFound) return EBTNodeResult::Failed; //유효한 위치를 찾지 못한 경우 실패 반환
	BlackboardComponent->SetValueAsVector(BlackboardKey.SelectedKeyName, OutLocation.Location); //블랙보드에 찾은 위치 설정
	return EBTNodeResult::Succeeded; //성공 반환
}