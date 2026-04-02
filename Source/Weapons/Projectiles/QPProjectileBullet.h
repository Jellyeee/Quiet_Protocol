#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QPProjectileBullet.generated.h"

/**
 * AQPProjectileBullet
 * 
 * 실제 총기에서 발사되는 물리 투사체 클래스입니다.
 * 중력의 영향을 받으며, 충돌 시 데미지를 전달하고 시각 효과(Niagara)를 관리합니다.
 * 네트워크 복제를 통해 모든 클라이언트에서 동일한 궤적을 그리도록 설계되었습니다.
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPProjectileBullet : public AActor
{
	GENERATED_BODY()

public:
	AQPProjectileBullet();
	/** 투사체의 초기 속도 및 방향 설정 (서버/클라이언트 공통) */
	void SetBulletVelocity(const FVector& Direction, float Speed); 

	/** [Network] 서버에서 설정된 초기 속도를 클라이언트로 복제하기 위한 변수 */
	UPROPERTY(ReplicatedUsing = OnRep_InitialVelocity)
	FVector ReplicatedInitialVelocity;

	/** 복제된 속도 값이 수신되었을 때 클라이언트에서 실행될 콜백 함수 */
	UFUNCTION()
	void OnRep_InitialVelocity();

	/** 네트워크 복제 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 투사체가 가할 데미지 수치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float Damage = 10.f;

	/** 데미지 타입 클래스 (물리, 폭발 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class UDamageType> DamageTypeClass;

	/** 타격 발생 시 호출되는 이벤트 핸들러 */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override; // 액터 파괴 시 호출되어 이펙트를 확실히 제거
	virtual void Tick(float DeltaTime) override;
	/** 충돌 감지를 위한 구체 컴포넌트 (Root) */
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class USphereComponent> BulletCollision; 

	/** 투사체 이동 로직(속도, 중력 등)을 관리하는 무브먼트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement; 

	/** 시각적으로 보여질 총알 메시 */
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<class USkeletalMeshComponent> BulletMesh; 

	/** 총알 뒤에 생기는 꼬리(Trail) 효과를 위한 나이아가라 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet|FX")
	TObjectPtr<class UNiagaraSystem> TrailFX = nullptr; 

	/** 트레일 효과의 크기 조절 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet|FX")
	FVector TrailFXScale = FVector(1.0f, 1.0f, 1.0f); 

	/** 런타임에 생성된 트레일 이펙트 인스턴스 */
	UPROPERTY(Transient) 
	TObjectPtr<class UNiagaraComponent> TrailFXComponent = nullptr; 
	//총알 Tracer 대체 임시 디버그	변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	bool bDebugDrawTracer = true; //디버그 트레이서 그리기 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	float DebugSegmentLifeTime = 1.0f; //디버그 세그먼트 수명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug", meta = (AllowPrivateAcess = "true"))
	float DebugThickness = 2.0f; //디버그 두께

	FVector PrevLocation = FVector::ZeroVector; //이전 위치

};
