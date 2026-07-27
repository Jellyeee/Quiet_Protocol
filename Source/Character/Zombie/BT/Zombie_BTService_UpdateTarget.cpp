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
	NodeName = TEXT("Update Target");
	Interval = 0.2f;
	RandomDeviation = 0.0f;
	
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, BlackboardKey), AActor::StaticClass());

	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, LastKnownLocationKey));
	InvestigatingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, InvestigatingKey));
	HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, HasTargetKey));
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, DistanceToTargetKey));
	IsAttackingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UZombie_BTService_UpdateTarget, IsAttackingKey));
}

static AActor* FindClosestPlayerPawn(UWorld* World, const FVector& From)
{
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (FConstPlayerControllerIterator it = World->GetPlayerControllerIterator(); it; ++it)
	{
		APlayerController* PlayerController = it->Get();
		if (!PlayerController) continue;
		APawn* Pawn = PlayerController->GetPawn();
		if (!IsValid(Pawn)) continue;
		const float DistSquard = FVector::DistSquared(From, Pawn->GetActorLocation());
		if (DistSquard < BestDistSq)
		{
			BestDistSq = DistSquard;
			Best = Pawn;
		}
	}
	return Best;
}

void UZombie_BTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (!AIController || !Pawn || !BlackboardComponent) return;

	AZombieCharacter* Zombie = Cast<AZombieCharacter>(Pawn);
	if (!Zombie) return;

	const FName KeyName = BlackboardKey.SelectedKeyName;
	const FName LastKnownLocationKeyName = LastKnownLocationKey.SelectedKeyName;
	const FName InvestigatingKeyName = InvestigatingKey.SelectedKeyName;
	const FName HasTargetKeyName = HasTargetKey.SelectedKeyName;
	const FName DistanceToTargetKeyName = DistanceToTargetKey.SelectedKeyName;
	const FName IsAttackingKeyName = IsAttackingKey.SelectedKeyName;

	const bool bWasInvestigating = BlackboardComponent->GetValueAsBool(InvestigatingKeyName);
	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(KeyName));
	AActor* CandidateTarget = CurrentTarget;
	if (!IsValid(CandidateTarget)) {
		CandidateTarget = FindClosestPlayerPawn(Pawn->GetWorld(), Pawn->GetActorLocation());
	}
	if (!IsValid(CandidateTarget)) {
		BlackboardComponent->ClearValue(KeyName);
		Zombie->SetTarget(nullptr);
		BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false);

		BlackboardComponent->SetValueAsBool(HasTargetKeyName, false);
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKeyName, TNumericLimits<float>::Max());
		BlackboardComponent->SetValueAsBool(IsAttackingKeyName, Zombie->IsAttacking());

		return;
	}
	const float DistanceToCandidate = FVector::Dist(Pawn->GetActorLocation(), CandidateTarget->GetActorLocation());
	const bool bHasTargetAlready = IsValid(CurrentTarget);
	const float RangeToUse = bHasTargetAlready ? LoseTargetRange : DetectRange;

	bool bPass = (DistanceToCandidate <= RangeToUse);

	if (bPass && bRequireLineOfSight)
	{
		bPass = AIController->LineOfSightTo(CandidateTarget);
	}
	if (bPass) {
		const bool bAcquiring = !bHasTargetAlready;
		const float FOV = bAcquiring ? DetectFovDegress : LoseFOVDegrees;
		if (FOV < 360.F)
		{
			const FVector Forward2D = AIController->GetControlRotation().Vector().GetSafeNormal2D();
			const FVector ToTarget2D = (CandidateTarget->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
			const float Dot = FVector::DotProduct(Forward2D, ToTarget2D);
			const float CosHalf = FMath::Cos(FMath::DegreesToRadians(FOV * 0.5f));

			if (Dot < CosHalf)
			{
				bPass = false;
			}
		}
	}
	if (bPass)
	{
		BlackboardComponent->SetValueAsObject(KeyName, CandidateTarget);
		Zombie->SetTarget(CandidateTarget);

		BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, CandidateTarget->GetActorLocation());
		BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false);
	}
	else
	{
		const bool bHadTarget = IsValid(CurrentTarget);
		BlackboardComponent->ClearValue(KeyName);
		Zombie->SetTarget(nullptr);
		if (bHadTarget)
		{
			BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, CurrentTarget->GetActorLocation());
			BlackboardComponent->SetValueAsBool(InvestigatingKeyName, true);
		}
		else {
			if (!bWasInvestigating)
			{
				BlackboardComponent->SetValueAsBool(InvestigatingKeyName, false);
			}
		}
	}
	AActor* FinalTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(KeyName));
	const bool bHasTargetNow = IsValid(FinalTarget);
	BlackboardComponent->SetValueAsBool(HasTargetKeyName, bHasTargetNow);
	BlackboardComponent->SetValueAsFloat(DistanceToTargetKeyName, bHasTargetNow ? FVector::Dist(Pawn->GetActorLocation(), FinalTarget->GetActorLocation()) : TNumericLimits<float>::Max());

	BlackboardComponent->SetValueAsBool(IsAttackingKeyName, Zombie->IsAttacking());
}
