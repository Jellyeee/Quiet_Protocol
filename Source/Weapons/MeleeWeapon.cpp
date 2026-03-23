// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

AMeleeWeapon::AMeleeWeapon()
{
	WeaponType = EQPWeaponType::EWT_Melee; //무기 타입을 근접 무기로 설정
	bAutomatic = false; // 근접 공격은 꾹 눌러도 연속 공격이 나가지 않도록 설정
	FireRate = 1.25f; // 근접 공격 연사 간격(쿨타임) 설정 - 애니메이션이 끝나기 전까지 다시 공격하지 못하도록 딜레이 적용
	BaseDamage = 40.f; // 기본 데미지 설정
}

void AMeleeWeapon::StartFire_Implementation() //공격 시작 함수 재정의
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); //무기 소유자를 캐릭터로 캐스팅
	if (!OwnerCharacter) return; //소유자가 유효하지 않으면 반환
	const FVector Start = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 50.f; //캐릭터 앞쪽으로 시작 지점 설정
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * SwingRange; //스윙 반경만큼 떨어진 지점을 끝 지점으로 설정
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeSwing), false); //충돌 쿼리 파라미터 설정
	Params.AddIgnoredActor(this); //자기 자신 무시
	Params.AddIgnoredActor(OwnerCharacter); //소유자 무시
	TArray<FHitResult> HitResults; //히트 결과 배열
	const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(SwingRadius), Params); //스윕 수행
	// DrawDebugSphere(GetWorld(), End, SwingRadius, 12, FColor::Green, false, 0.35f); //디버그 스피어
	if (!bHit) return;	//히트하지 않았으면 반환
	for (const FHitResult& Hit : HitResults)//히트 결과 반복
	{
		AActor* HitActor = Hit.GetActor(); //히트한 액터 가져오기
		if (!HitActor) continue; //히트한 액터가 유효하지 않으면 다음으로
		UGameplayStatics::ApplyPointDamage(HitActor, BaseDamage, OwnerCharacter->GetActorForwardVector(), Hit, OwnerCharacter->GetController(),
			this, DamageTypeClass); //데미지 적용
		break; //첫 번째 히트한 액터에만 데미지 적용 후 종료
	}

}
