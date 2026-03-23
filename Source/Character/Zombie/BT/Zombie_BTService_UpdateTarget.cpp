#include "Zombie_BTService_UpdateTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Character/Zombie/ZombieCharacter.h"
//#include "PJ_Quiet_Protocol/Commons/DefineCommons.h"


UZombie_BTService_UpdateTarget::UZombie_BTService_UpdateTarget()
{
	NodeName = TEXT("Update Target"); //노드 이름 설정
	Interval = 0.2f; //틱 간격 설정
	RandomDeviation = 0.0f; //랜덤 편차 설정
	// BT에서 Object 키(TargetActor)로 지정할 거라서 타입 제한을 걸어두면 실수 줄어듦
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, BlackboardKey), AActor::StaticClass()); //블랙보드 키에 AActor 타입 필터 추가

	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, LastKnownLocationKey)); //마지막 알려진 위치 키에 벡터 타입 필터 추가
	InvestigatingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, InvestigatingKey)); //조사 중 키에 불린 타입 필터 추가
	HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, HasTargetKey)); //타겟 보유 키에 불린 타입 필터 추가
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, DistanceToTargetKey)); //타겟과의 거리 키에 플로트
	IsAttackingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, IsAttackingKey)); //공격 중 키에 불린 타입 필터 추가

}

static AActor* FindClosestPlayerPawn(UWorld* World, const FVector& From) //가장 가까운 플레이어 폰 찾기 함수
{
	AActor* Best = nullptr; //최적의 타겟 초기화
	float BestDistSq = TNumericLimits<float>::Max(); //최적 거리 제곱 초기화
	// 월드에 존재하는 플레이어 캐릭터들을 훑음
	for (FConstPlayerControllerIterator it = World->GetPlayerControllerIterator(); it; ++it)
	{
		APlayerController* PlayerController = it->Get(); //플레이어 컨트롤러 가져오기
		if (!PlayerController) continue; //유효성 검사	
		APawn* Pawn = PlayerController->GetPawn(); //플레이어 폰 가져오기
		if (!IsValid(Pawn)) continue; //유효성 검사
		const float DistSquard = FVector::DistSquared(From, Pawn->GetActorLocation()); //거리 제곱 계산
		if (DistSquard < BestDistSq) //최적 거리보다 가까운 경우
		{
			BestDistSq = DistSquard; //최적 타겟 갱신
			Best = Pawn; //최적 거리 갱신
		}
	}
	return Best; //최적 타겟 반환
}

void UZombie_BTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds); //부모 클래스의 TickNode 호출

	AAIController* AIController = OwnerComp.GetAIOwner(); //AI 컨트롤러 가져오기
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr; //AI 컨트롤러의 폰 가져오기
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent(); //블랙보드 컴포넌트 가져오기

	if (!AIController || !Pawn || !BlackboardComponent) return; //유효성 검사

	AZombieCharacter* Zombie = Cast<AZombieCharacter>(Pawn); //좀비 캐릭터로 캐스팅

	if (!Zombie) return; //좀비 캐릭터 유효성 검사

	const FName KeyName = BlackboardKey.SelectedKeyName; //블랙보드 키 이름 가져오기
	const FName LastKnownLocationKeyName = LastKnownLocationKey.SelectedKeyName; //마지막 알려진 위치 키 이름 가져오기
	const FName InvestigatingKeyName = InvestigatingKey.SelectedKeyName; //조사 중	
	const FName HasTargetKeyName = HasTargetKey.SelectedKeyName; //타겟 보유 키 이름
	const FName DistanceToTargetKeyName = DistanceToTargetKey.SelectedKeyName; //타겟과의 거리 키 이름
	const FName IsAttackingKeyName = IsAttackingKey.SelectedKeyName; //공격 중 키 이름

	const bool bWasInvestigating = BlackboardComponent->GetValueAsBool(InvestigatingKeyName); //이전 조사 중 상태 가져오기
	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(KeyName)); //현재 타겟 가져오기
	//타겟 후보 선정
	AActor* CandidateTarget = CurrentTarget; //타겟 후보 초기화
	if (!IsValid(CandidateTarget)) {
		CandidateTarget = FindClosestPlayerPawn(Pawn->GetWorld(), Pawn->GetActorLocation()); //가장 가까운 플레이어 폰 찾기
	}
	if (!IsValid(CandidateTarget)) {
		BlackboardComponent->ClearValue(KeyName); //유효하지 않은 후보인 경우 블랙보드에서 타겟 제거
		Zombie->SetTarget(nullptr); //좀비의 타겟도 제거
		BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false); //조사 중 해제

		BlackboardComponent->SetValueAsBool(HasTargetKeyName, false); //타겟 보유 상태 갱신
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKeyName, TNumericLimits<float>::Max()); //타겟과의 거리 갱신
		BlackboardComponent->SetValueAsBool(IsAttackingKeyName, Zombie->IsAttacking()); //공격 중 상태

		return;
	}
	//거리조건
	const float DistanceToCandidate = FVector::Dist(Pawn->GetActorLocation(), CandidateTarget->GetActorLocation()); //후보와의 거리 계산
	const bool bHasTargetAlready = IsValid(CurrentTarget); //현재 타겟이 있는지 여부
	const float RangeToUse = bHasTargetAlready ? LoseTargetRange : DetectRange; //타겟 유지/탐지 범위 설정

	bool bPass = (DistanceToCandidate <= RangeToUse); //거리 기반 통과 여부 판단

	if (bPass && bRequireLineOfSight) //거리 조건 통과 시 시야 조건 검사
	{
		bPass = AIController->LineOfSightTo(CandidateTarget); //시야 조건 통과 여부 판단
	}
	if (bPass) {
		const bool bAcquiring = !bHasTargetAlready; //타겟 획득 여부
		const float FOV = bAcquiring ? DetectFovDegress : LoseFOVDegrees; //시야각 설정
		if (FOV < 360.F) //시야각이 360도 미만인 경우
		{
			const FVector Forward2D = AIController->GetControlRotation().Vector().GetSafeNormal2D(); //전방 벡터 (2D)
			const FVector ToTarget2D = (CandidateTarget->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D(); //후보 타겟 방향 벡터 (2D)
			const float Dot = FVector::DotProduct(Forward2D, ToTarget2D); //두 벡터의 내적 계산
			const float CosHalf = FMath::Cos(FMath::DegreesToRadians(FOV * 0.5f)); //시야각의 절반 코사인 값 계산

			if (Dot < CosHalf) //내적이 코사인 값보다 작은 경우 (시야각 밖에 있는 경우)
			{
				bPass = false; //시야 조건 미통과
			}

		}
	}
	if (bPass) //모든 조건 통과 시 타겟 갱신
	{
		BlackboardComponent->SetValueAsObject(KeyName, CandidateTarget); //블랙보드에 타겟 설정
		Zombie->SetTarget(CandidateTarget); //좀비의 타겟 설정

		BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, CandidateTarget->GetActorLocation()); //마지막 알려진 위치 갱신
		BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false); //조사 중 해제
	}
	else
	{
		const bool bHadTarget = IsValid(CurrentTarget); //이전 타겟이 있었는지 여부
		BlackboardComponent->ClearValue(KeyName); //조건 미통과 시 블랙보드에서 타겟 제거
		Zombie->SetTarget(nullptr); //좀비의 타겟도 제거
		if (bHadTarget) //이전 타겟이 있었던 경우
		{
			BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, CurrentTarget->GetActorLocation()); //마지막 알려진 위치 갱신
			BlackboardComponent->SetValueAsBool(InvestigatingKeyName, true); //조사 중으로 설정
		}
		else {
			if (!bWasInvestigating)
			{
				BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false); //조사 중 해제
			}
		}
	}
	AActor* FinalTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(KeyName)); //최종 타겟 가져오기
	const bool bHasTargetNow = IsValid(FinalTarget); //최종 타겟이 있는지 여부
	BlackboardComponent->SetValueAsBool(HasTargetKeyName, bHasTargetNow); //타겟 보유 상태 갱신
	BlackboardComponent->SetValueAsFloat(DistanceToTargetKeyName, bHasTargetNow ? FVector::Dist(Pawn->GetActorLocation(), FinalTarget->GetActorLocation()) : TNumericLimits<float>::Max()); //타겟과의 거리 갱신

	BlackboardComponent->SetValueAsBool(IsAttackingKeyName, Zombie->IsAttacking()); //공격 중 상태

	//DBG_SCREEN(
	//	1002, 0.25f, FColor::Yellow,
	//	"Keys: TargetKey=%s HasTargetKey=%s DistKey=%s IsAttackingKey=%s",
	//	*KeyName.ToString(),
	//	*HasTargetKeyName.ToString(),
	//	*DistanceToTargetKeyName.ToString(),
	//	*IsAttackingKeyName.ToString()
	//);
}