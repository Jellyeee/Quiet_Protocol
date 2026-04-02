#include "ZombieAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"

AZombieAIController::AZombieAIController()
{
	// AI 퍼셉션 컴포넌트 생성
	ZombiePerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("ZombiePerceptionComponent"));
	
	// 청각 감지(Hearing) 감각 설정
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	HearingConfig->HearingRange = 1500.0f; // 청각 반경 설정
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true; // 적군 감지 활성화
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true; // 아군 감지 활성화
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true; // 중립 감지 활성화

	// 퍼셉션 컴포넌트에 청각 감각 등록
	ZombiePerceptionComponent->ConfigureSense(*HearingConfig);
	// 주 감각(Dominant Sense)으로 청각 설정
	ZombiePerceptionComponent->SetDominantSense(HearingConfig->GetSenseImplementation());

	// 타겟 퍼셉션 업데이트 시 호출될 델리게이트 바인딩
	ZombiePerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnTargetPerceptionUpdated);
}

void AZombieAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 자극(Stimulus)이 성공적으로 감지되었는지 확인
	if (Stimulus.WasSuccessfullySensed())
	{
		// 블랙보드 컴포넌트가 유효한지 확인
		if (GetBlackboardComponent())
		{
			// 소리가 발생한 위치(StimulusLocation)를 조사 위치로 기록
			GetBlackboardComponent()->SetValueAsVector(TEXT("InvestigateLocation"), Stimulus.StimulusLocation);
		}
	}
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (BehaviorTreeAsset) {
		RunBehaviorTree(BehaviorTreeAsset); //행동 트리 실행
	}
}