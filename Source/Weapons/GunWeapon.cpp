// Fill out your copyright notice in the Description page of Project Settings.


#include "GunWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Projectiles/QPProjectileBullet.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"

AGunWeapon::AGunWeapon()
{
	WeaponType = EQPWeaponType::EWT_Rifle; //기본 무기 타입을 소총으로 설정 (추후 자식 클래스에서 변경 가능)
}

void AGunWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 샷건과 권총은 기본적으로 단발(반자동) 무기로 취급하여 버튼을 꾹 눌러도 연사되지 않도록 설정
	if (WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun)
	{
		bAutomatic = false;
	}
}

void AGunWeapon::StartFire_Implementation() //발사 시작 함수 재정의
{
	FireOnce(); //한 번 발사
}

void AGunWeapon::StopAttack_Implementation() //공격 중지 함수 재정의
{
	// 기본 구현 (필요시 추가)
}

//임시 코드 다음에 프로젝트일 만들어서 총알 나가게 수정 현재는 히트스캔
void AGunWeapon::FireOnce() //한 번 발사 함수
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); //무기 소유자를 캐릭터로 캐스팅
	if (!OwnerCharacter) return; //소유자가 유효하지 않으면 반환

	PelletsFiredCount = 0; // 발사된 총알 수 초기화

	if (WeaponType == EQPWeaponType::EWT_Shotgun)
	{
		// 이전에 실행 중이던 타이머가 있다면 초기화하여 겹치는 현상 방지
		GetWorldTimerManager().ClearTimer(ShotgunFireTimerHandle);
		
		// 지정된 간격(TimeBetweenPellets)마다 FireSinglePellet을 반복 호출
		GetWorldTimerManager().SetTimer(ShotgunFireTimerHandle, this, &AGunWeapon::FireSinglePellet, TimeBetweenPellets, true, 0.f);
	}
	else
	{
		// 샷건이 아닌 무기(권총, 소총 등)는 단 한 발만 즉시 발사
		FireSinglePellet();
	}
}

void AGunWeapon::FireSinglePellet()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); //무기 소유자를 캐릭터로 캐스팅
	if (!OwnerCharacter) return; //소유자가 유효하지 않으면 반환

	// 무기 타입에 따른 사거리(Range) 및 퍼짐 각도 설정
	float CurrentRange = Range;
	float CurrentSpreadAngle = 0.f;

	if (WeaponType == EQPWeaponType::EWT_Handgun)
	{
		CurrentRange = HandgunRange; // 권총은 사거리가 짧음
	}
	else if (WeaponType == EQPWeaponType::EWT_Shotgun)
	{
		CurrentSpreadAngle = ShotgunSpreadAngle; // 샷건 퍼짐 각도 적용
	}

	// 소유 캐릭터의 컴포넌트에서 QPCombatComponent를 찾아서 크로스헤어 타겟 정보(HitTarget)를 가져옴
	UQPCombatComponent* CombatComponent = OwnerCharacter->FindComponentByClass<UQPCombatComponent>();
	if (!CombatComponent) return;

	// 크로스헤어 타겟 (HitTarget)
	const FVector TraceEnd = CombatComponent->HitTarget;

	//Bullet Projectile Spawn
	FVector MuzzleLocation = GetActorLocation(); //기본 총구 위치를 액터 위치로 설정
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))//무기 메쉬와 총구 소켓이 유효한 경우
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName); //총구 소켓 위치 가져오기
	}

	// 총알 방향 계산 (총구에서 타겟까지의 방향 벡터)
	const FVector BaseBulletDir = (TraceEnd - MuzzleLocation).GetSafeNormal();
	FVector FinalBulletDir = BaseBulletDir;

	// 샷건일 경우 퍼짐(Spread) 적용
	if (CurrentSpreadAngle > 0.f)
	{
		// 원뿔 형태로 랜덤하게 방향 벡터를 틀어줌 (HalfAngle 형태이므로 FMath::DegreesToRadians 사용)
		float HalfAngleRad = FMath::DegreesToRadians(CurrentSpreadAngle);
		FinalBulletDir = FMath::VRandCone(BaseBulletDir, HalfAngleRad);
	}

	// 사거리 제한 적용 (목표점이 너무 멀면 방향만 유지하고 최대 사거리까지만 쏘도록)
	FVector FinalTarget = MuzzleLocation + (FinalBulletDir * CurrentRange);

	DrawDebugLine(GetWorld(), MuzzleLocation, FinalTarget, FColor::Red, false, 2.0f, 0, 2.0f); //디버그 라인이 총구에서 나가도록 그리기

	FActorSpawnParameters SpawnParams; //스폰 파라미터 설정
	SpawnParams.Owner = OwnerCharacter; //소유자 설정
	SpawnParams.Instigator = OwnerCharacter; //인스티게이터 설정

	const FRotator SpawnRotation = FinalBulletDir.Rotation(); //발사 방향을 회전으로 변환
	AQPProjectileBullet* ProjectileBullet = GetWorld()->SpawnActor<AQPProjectileBullet>(ProjectileBulletClass, MuzzleLocation, SpawnRotation, SpawnParams); //투사체 불릿 스폰
	
	if (ProjectileBullet) 
	{
		ProjectileBullet->SetBulletVelocity(FinalBulletDir, BulletSpeed); //계산된 총알 방향으로 속도 설정
		ProjectileBullet->Damage = BaseDamage;
		ProjectileBullet->DamageTypeClass = DamageTypeClass;
	}

	// 샷건일 경우 지정된 총알 수만큼 쐈는지 확인하고 타이머 종료
	if (WeaponType == EQPWeaponType::EWT_Shotgun)
	{
		PelletsFiredCount++;
		if (PelletsFiredCount >= ShotgunPelletCount)
		{
			GetWorldTimerManager().ClearTimer(ShotgunFireTimerHandle);
		}
	}

	/*if (bHit && Hit.GetActor()) //히트했으며 히트한 액터가 유효한 경우
	{

		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), BaseDamage, Dir, Hit, OwnerCharacter->GetInstigatorController(), this, DamageTypeClass);//데미지 적용

	}*/
}
