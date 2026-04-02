#include "BTTask_ZombieStartAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Character/Zombie/ZombieCharacter.h"
//#include "PJ_Quiet_Protocol/Commons/DefineCommons.h"

UBTTask_ZombieStartAttack::UBTTask_ZombieStartAttack()
{
	NodeName = TEXT("Zombie Start Attack");
	isAttackingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZombieStartAttack, isAttackingKey));
}

EBTNodeResult::Type UBTTask_ZombieStartAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AZombieCharacter* Zombie = (AIController ? Cast<AZombieCharacter>(AIController->GetPawn()) : nullptr);

	if (!AIController || !BlackboardComponent || !Zombie) return EBTNodeResult::Failed;

	AIController->StopMovement();
	if (Zombie->GetCharacterMovement()) Zombie->GetCharacterMovement()->StopMovementImmediately();
	Zombie->StartAttack();

	if (!isAttackingKey.SelectedKeyName.IsNone()) BlackboardComponent->SetValueAsBool(isAttackingKey.SelectedKeyName, Zombie->IsAttacking());

	return EBTNodeResult::Succeeded;
}