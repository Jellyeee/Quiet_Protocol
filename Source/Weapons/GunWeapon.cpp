#include "GunWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Projectiles/QPProjectileBullet.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"

AGunWeapon::AGunWeapon()
{
	WeaponType = EQPWeaponType::EWT_Rifle;
}

void AGunWeapon::BeginPlay()
{
	Super::BeginPlay();

	/** 
	 * [무기별 커스터마이징 초기화]
	 * 샷건과 권총은 연사(Full-Auto) 시 밸런스 및 조작감을 위해 반자동 무기로 설정합니다.
	 */
	if (WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun)
	{
		bAutomatic = false;
	}

	/** 무기 타입에 따른 기본 장탄수 및 연사 간격(FireRate) 보정 */
	if (WeaponType == EQPWeaponType::EWT_Rifle) 
	{
		MagCapacity = 30;
	}
	else if (WeaponType == EQPWeaponType::EWT_Handgun) 
	{
		MagCapacity = 10;
		// 권총의 발사 간격이 너무 빠르면 0.3초로 제한하여 안정성 확보
		if (FMath::IsNearlyEqual(FireRate, 0.15f)) FireRate = 0.3f; 
	}
	else if (WeaponType == EQPWeaponType::EWT_Shotgun) 
	{
		MagCapacity = 4;
		// 샷건은 강력한 화력을 고려해 발사 간격을 0.8초로 길게 설정
		if (FMath::IsNearlyEqual(FireRate, 0.15f)) FireRate = 0.8f; 
	}

	CurrentAmmo = MagCapacity;
}

void AGunWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGunWeapon, CurrentAmmo);
}

void AGunWeapon::StartFire_Implementation() 
{
	/** 발사 가능 조건 확인: 탄약이 있어야 함 */
	if (CurrentAmmo <= 0) return; 

	/** 한 발 소비 후 실제 발사 로직 수행 */
	SpendRound(); 
	FireOnce();
	
	/**
	 * [AI 소음 보고 시스템]
	 * 총기 발사 시 주변 AI(좀비 등)가 감지할 수 있는 소음을 발생시킵니다.
	 * 무기 종류에 따라 소음의 크기(Loudness)를 다르게 설정합니다.
	 */
	if (GetOwner())
	{
		// 기본적인 '뛰는 소리(0.5)' 배율을 기준으로 설정
		float NoiseLoudness = 1.0f; 

		if (WeaponType == EQPWeaponType::EWT_Handgun) 
		{
			NoiseLoudness = 2.0f; // 권총: 뛰는 소리의 2배
		}
		else if (WeaponType == EQPWeaponType::EWT_Rifle || WeaponType == EQPWeaponType::EWT_Shotgun)
		{
			NoiseLoudness = 4.0f; // 라이플 / 샷건: 뛰는 소리의 4배
		}

		// AISense_Hearing을 통해 월드에 소음 이벤트 보고
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), NoiseLoudness, Cast<APawn>(GetOwner()), 0.f, TEXT("WeaponNoise"));
	}
}

void AGunWeapon::StopAttack_Implementation() //공격 중지 함수 재정의
{
	// 기본 구현 (필요시 추가)
}

void AGunWeapon::FireOnce()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); 
	if (!OwnerCharacter) return; 

	PelletsFiredCount = 0; 

	if (WeaponType == EQPWeaponType::EWT_Shotgun)
	{
		/** 
		 * 샷건은 한 프레임에 여러 개의 산탄을 부채꼴 형태로 동시 발사합니다.
		 * 타이머를 사용하지 않고 루프를 통해 즉시 다수의 Pellets을 생성합니다.
		 */
		for (int32 i = 0; i < ShotgunPelletCount; ++i)
		{
			FireSinglePellet();
		}
	}
	else
	{
		// 일반 총기는 단 한 개의 투사체만 즉시 발사
		FireSinglePellet();
	}
}

void AGunWeapon::FireSinglePellet()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return; 

	// 1. 무기 타입에 따른 사거리 및 퍼짐 각도 결정
	float CurrentRange = Range;
	float CurrentSpreadAngle = 0.f;

	if (WeaponType == EQPWeaponType::EWT_Handgun)
	{
		CurrentRange = HandgunRange; 
	}
	else if (WeaponType == EQPWeaponType::EWT_Shotgun)
	{
		CurrentSpreadAngle = ShotgunSpreadAngle; 
	}

	/** QPCombatComponent에서 계산된 크로스헤어 타겟(HitTarget)을 가져옵니다. */
	UQPCombatComponent* CombatComponent = OwnerCharacter->FindComponentByClass<UQPCombatComponent>();
	if (!CombatComponent) return;

	/** 화면 중앙(크로스헤어)이 가리키는 월드 좌표 */
	const FVector TraceEnd = CombatComponent->HitTarget;

	// 2. 총구 위치(Muzzle) 계산
	FVector MuzzleLocation = GetActorLocation(); 
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName); 
	}

	// 3. 발사 방향 계산
	const FVector BaseBulletDir = (TraceEnd - MuzzleLocation).GetSafeNormal();
	FVector FinalBulletDir = BaseBulletDir;

	/** 샷건일 경우 설정된 각도 내에서 랜덤하게 방향을 틀어 탄 퍼짐 구현 */
	if (CurrentSpreadAngle > 0.f)
	{
		float HalfAngleRad = FMath::DegreesToRadians(CurrentSpreadAngle);
		FinalBulletDir = FMath::VRandCone(BaseBulletDir, HalfAngleRad);
	}

	// 4. 사거리 제한을 적용한 최종 타겟 위치 (필요시 트레이스 용도로 활용 가능)
	FVector FinalTarget = MuzzleLocation + (FinalBulletDir * CurrentRange);

	// 5. 서버/클라이언트 공통 투사체 스폰 로직
	FActorSpawnParameters SpawnParams; 
	SpawnParams.Owner = OwnerCharacter; 
	SpawnParams.Instigator = OwnerCharacter; 
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; 

	const FRotator SpawnRotation = FinalBulletDir.Rotation(); 
	AQPProjectileBullet* ProjectileBullet = GetWorld()->SpawnActor<AQPProjectileBullet>(ProjectileBulletClass, MuzzleLocation, SpawnRotation, SpawnParams); 
	
	if (ProjectileBullet) 
	{
		/** 생성된 투사체에 속도 및 데미지 정보 전달 */
		ProjectileBullet->SetBulletVelocity(FinalBulletDir, BulletSpeed); 
		ProjectileBullet->Damage = BaseDamage;
		ProjectileBullet->DamageTypeClass = DamageTypeClass;
	}
}

void AGunWeapon::AddAmmo(int32 AmountToAdd)
{
	CurrentAmmo = FMath::Clamp(CurrentAmmo + AmountToAdd, 0, MagCapacity);
}

void AGunWeapon::SpendRound()
{
	CurrentAmmo = FMath::Clamp(CurrentAmmo - 1, 0, MagCapacity);
}
