#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPProjectileBullet.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AQPProjectileBullet : public AActor
{
	GENERATED_BODY()

public:
	AQPProjectileBullet();
	void SetBulletVelocity(const FVector& Direction, float Speed); //총알 속도 설정 함수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class USphereComponent> BulletCollision; //총알 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement; //총알 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class USkeletalMeshComponent> BulletMesh; //총알 메쉬 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet|FX")
	TObjectPtr<class UNiagaraSystem> TrailFX = nullptr; //충돌 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet|FX")
	FVector TrailFXScale = FVector(1.0f, 1.0f, 1.0f); //트레일 이펙트 스케일
	UPROPERTY(Transient) //임시
		TObjectPtr<class UNiagaraComponent> TrailFXComponent = nullptr; //트레일
	//총알 Tracer 대체 임시 디버그	변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	bool bDebugDrawTracer = true; //디버그 트레이서 그리기 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	float DebugSegmentLifeTime = 1.0f; //디버그 세그먼트 수명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	float DebugThickness = 2.0f; //디버그 두께

	FVector PrevLocation = FVector::ZeroVector; //이전 위치

};
