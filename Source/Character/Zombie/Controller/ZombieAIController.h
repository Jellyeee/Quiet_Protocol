#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZombieAIController.generated.h"

class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AZombieAIController : public AAIController
{
	GENERATED_BODY()
public:
	AZombieAIController();
protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

public:
	UPROPERTY(EditAnywhere, Category = "Zombie AI")
	UBehaviorTree* BehaviorTreeAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie AI|Perception")
	UAIPerceptionComponent* ZombiePerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie AI|Perception")
	UAISenseConfig_Hearing* HearingConfig;
};