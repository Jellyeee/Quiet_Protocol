#include "MeleeWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

AMeleeWeapon::AMeleeWeapon()
{
	WeaponType = EQPWeaponType::EWT_Melee; // 무기 타입을 근접 무기로 설정
	bAutomatic = false; // 단발 공격으로 설정
	FireRate = 1.25f; // 공격 속도 설정
	BaseDamage = 40.f; // 기본 데미지 설정
}

void AMeleeWeapon::StartFire_Implementation()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 공격 시작 지점 계산 (캐릭터 앞쪽 50.f 위치)
	const FVector Start = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 50.f;
	// 공격 끝 지점 계산 (기본 범위만큼 앞쪽)
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * SwingRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeSwing), false);
	Params.AddIgnoredActor(this); // 무기 자신 무시
	Params.AddIgnoredActor(OwnerCharacter); // 공격한 캐릭터 무시

	TArray<FHitResult> HitResults;
	// 스피어 스윕(Sphere Sweep)을 이용한 다중 충돌 검사
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults, 
		Start, 
		End, 
		FQuat::Identity, 
		ECC_Pawn, 
		FCollisionShape::MakeSphere(SwingRadius), 
		Params
	);

	if (!bHit) return;

	// 충돌 결과 순회
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		// 타겟에게 데미지 적용
		UGameplayStatics::ApplyPointDamage(
			HitActor, 
			BaseDamage, 
			OwnerCharacter->GetActorForwardVector(), 
			Hit, 
			OwnerCharacter->GetController(),
			this, 
			DamageTypeClass
		);
		// 첫 번째로 맞은 대상에게만 데미지를 주고 종료 (관통 공격이 아님)
		break;
	}
}
