#include "BTTask_ClearInvestigating.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearInvestigating::UBTTask_ClearInvestigating()
{
	NodeName = TEXT("Clear Investigating");
}


EBTNodeResult::Type UBTTask_ClearInvestigating::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent) return EBTNodeResult::Failed;

	BlackboardComponent->ClearValue(InvestigatingKey.SelectedKeyName);
	return EBTNodeResult::Succeeded;
}