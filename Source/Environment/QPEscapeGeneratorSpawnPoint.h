#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPEscapeGeneratorSpawnPoint.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AQPEscapeGeneratorSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AQPEscapeGeneratorSpawnPoint();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* BillboardComp;
};
