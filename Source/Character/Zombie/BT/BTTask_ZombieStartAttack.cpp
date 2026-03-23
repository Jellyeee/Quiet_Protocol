#include "BTTask_ZombieStartAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Character/Zombie/ZombieCharacter.h"
//#include "PJ_Quiet_Protocol/Commons/DefineCommons.h"

UBTTask_ZombieStartAttack::UBTTask_ZombieStartAttack()
{
	NodeName = TEXT("Zombie Start Attack"); //노드 이름 설정
	isAttackingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZombieStartAttack, isAttackingKey)); //블랙보드 키에 불린 타입 필
}

EBTNodeResult::Type UBTTask_ZombieStartAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner(); //AI 컨트롤러 가져오기
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent(); //블랙보드 컴포넌트 가져오기
	AZombieCharacter* Zombie = (AIController ? Cast<AZombieCharacter>(AIController->GetPawn()) : nullptr); //좀비 캐릭터로 캐스팅

	if (!AIController || !BlackboardComponent || !Zombie) return EBTNodeResult::Failed; //유효성 검사

	AIController->StopMovement(); //AI 컨트롤러의 이동 정지
	if (Zombie->GetCharacterMovement()) Zombie->GetCharacterMovement()->StopMovementImmediately(); //좀비의 이동 즉시 정지
	//DBG_SCREEN(
	//	2001, 1.0f, FColor::Red,
	//	"Task: StartAttack EXEC. Pawn=%s IsAttacking(before)=%d",
	//	*GetNameSafe(Zombie),
	//	Zombie->IsAttacking() ? 1 : 0
	//);
	Zombie->StartAttack(); //좀비 공격 시작

	//DBG_SCREEN(
	//	2002, 1.0f, FColor::Red,
	//	"Task: StartAttack DONE. IsAttacking(after)=%d",
	//	Zombie->IsAttacking() ? 1 : 0);

	if (!isAttackingKey.SelectedKeyName.IsNone()) BlackboardComponent->SetValueAsBool(isAttackingKey.SelectedKeyName, Zombie->IsAttacking()); //블랙보드에 공격중 플래그 설정

	return EBTNodeResult::Succeeded;
}