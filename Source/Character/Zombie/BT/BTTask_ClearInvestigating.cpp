#include "BTTask_ClearInvestigating.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearInvestigating::UBTTask_ClearInvestigating()
{
	NodeName = TEXT("Clear Investigating"); //노드 이름 설정
	InvestigatingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ClearInvestigating, InvestigatingKey)); //블랙보드 키에 불린 타입 필터 추가)
}


EBTNodeResult::Type UBTTask_ClearInvestigating::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent) return EBTNodeResult::Failed;

	BlackboardComponent->SetValueAsBool(InvestigatingKey.SelectedKeyName, false);
	return EBTNodeResult::Succeeded;
}