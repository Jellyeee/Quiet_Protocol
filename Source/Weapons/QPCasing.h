#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPCasing.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AQPCasing : public AActor
{
	GENERATED_BODY()
	
public:	
	AQPCasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere, Category = "Casing")
	float ShellEjectionImpulse = 10.f;

	UPROPERTY(EditAnywhere, Category = "Casing")
	class USoundBase* ShellSound;

	// 풀링을 위한 활성화 상태
	bool bIsActive = false;
	
	// 탄피 배출 (활성화)
	void ActivateCasing(const FTransform& EjectTransform, const FVector& WeaponVelocity);
	
	// 탄피 비활성화 (풀 반환)
	void DeactivateCasing();

private:
	FTimerHandle DeactivateTimer;
	bool bHasHitGround = false;
};
