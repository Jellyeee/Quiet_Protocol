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
		
		// 샷건 반동 키우기
		RecoilPitchMin = FMath::Max(RecoilPitchMin, 2.5f);
		RecoilPitchMax = FMath::Max(RecoilPitchMax, 4.0f);
		RecoilYawMin = FMath::Min(RecoilYawMin, -1.0f);
		RecoilYawMax = FMath::Max(RecoilYawMax, 1.0f);
	}

	CurrentAmmo = MagCapacity;

	// 탄피 풀 초기화 (로컬 클라이언트/리슨 서버 전용)
	InitializeCasingPool();
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

	/** QPCombatComponent에서 계산된 크로스헤어 타겟(HitTarget)과 현재 확산(Spread) 값을 가져옵니다. */
	UQPCombatComponent* CombatComponent = OwnerCharacter->FindComponentByClass<UQPCombatComponent>();
	if (!CombatComponent) return;

	/** 화면 중앙(크로스헤어)이 가리키는 월드 좌표 */
	FVector TraceEnd = CombatComponent->HitTarget;
	const float CombatSpread = CombatComponent->GetCrosshairSpread(); // [Add] 현재 동적 확산 값 가져오기

	// 2. 총구 위치(Muzzle) 계산 (타격감을 위해 다시 총구에서 스폰)
	FVector MuzzleLocation = GetActorLocation(); 
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName); 
	}

	// 3. 발사 방향 계산 (총구 -> 화면 중앙 크로스헤어 조준점 TraceEnd)
	const FVector BaseBulletDir = (TraceEnd - MuzzleLocation).GetSafeNormal();
	FVector FinalBulletDir = BaseBulletDir;

	/** 
	 * [Change] 크로스헤어 확산 상태를 실제 탄 퍼짐 각도로 변환하여 적용 
	 * 크로스헤어가 벌어진 정도(CombatSpread)에 비례하여 발사 각도를 랜덤하게 편향시킵니다.
	 */
	float TotalSpreadAngle = CurrentSpreadAngle;
	if (CombatSpread > 0.f)
	{
		// 픽셀 단위 확산값을 각도(Degrees)로 맵핑 (최대 확산 시 약 5~8도 정도 퍼지도록 설정)
		float DynamicSpreadAngle = (CombatSpread / CombatComponent->GetCrosshairSpreadMax()) * 6.0f;
		TotalSpreadAngle += DynamicSpreadAngle;
	}

	if (TotalSpreadAngle > 0.f)
	{
		float HalfAngleRad = FMath::DegreesToRadians(TotalSpreadAngle);
		FinalBulletDir = FMath::VRandCone(BaseBulletDir, HalfAngleRad);
	}

	// 4. 사거리 제한을 적용한 최종 타겟 위치 (필요시 트레이스 용도로 활용 가능)
	FVector FinalTarget = MuzzleLocation + (FinalBulletDir * CurrentRange);

	// [Point-Blank Check] 초근접(영거리) 사격 보정
	// 플레이어 카메라/가슴부터 총구 앞 범위 내에 적(좀비/타 플레이어)이 밀착해 있는 경우,
	// 총알이 총구 끝에서 스폰되어 적의 뒤쪽으로 발사되는 현상을 방지
	FHitResult PointBlankHit;
	FCollisionQueryParams PBParams(SCENE_QUERY_STAT(PointBlankCheck), false);
	PBParams.AddIgnoredActor(this);
	PBParams.AddIgnoredActor(OwnerCharacter);

	FVector CameraLoc;
	FRotator CameraRot;
	if (OwnerCharacter->GetController())
	{
		OwnerCharacter->GetController()->GetPlayerViewPoint(CameraLoc, CameraRot);
	}
	else
	{
		CameraLoc = OwnerCharacter->GetActorLocation();
	}

	const FVector PBStart = CameraLoc;
	const FVector PBEnd = MuzzleLocation + (FinalBulletDir * 100.f);

	bool bPointBlankHit = GetWorld()->LineTraceSingleByChannel(
		PointBlankHit,
		PBStart,
		PBEnd,
		ECC_Pawn,
		PBParams
	);

	if (bPointBlankHit && PointBlankHit.GetActor() && PointBlankHit.GetActor() != OwnerCharacter)
	{
		if (OwnerCharacter->HasAuthority())
		{
			UGameplayStatics::ApplyPointDamage(
				PointBlankHit.GetActor(),
				BaseDamage,
				FinalBulletDir,
				PointBlankHit,
				OwnerCharacter->GetController(),
				this,
				DamageTypeClass
			);
		}
		// 총알 스폰 위치를 히트 지점 근처로 조정하여 타격 효과가 즉시 발동되도록 설정
		MuzzleLocation = PointBlankHit.ImpactPoint - (FinalBulletDir * 15.f);
	}

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

void AGunWeapon::InitializeCasingPool()
{
	if (!CasingClass) return; // 블루프린트에서 클래스가 할당되지 않았으면 무시

	// 데디케이티드 서버에서는 비주얼적인 탄피를 스폰할 필요가 없음 (최적화)
	if (GetNetMode() == NM_DedicatedServer) return;

	for (int32 i = 0; i < CasingPoolSize; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		// 보이지 않는 위치에 임시 생성
		AQPCasing* NewCasing = GetWorld()->SpawnActor<AQPCasing>(CasingClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (NewCasing)
		{
			CasingPool.Add(NewCasing);
		}
	}
}

AQPCasing* AGunWeapon::GetAvailableCasing()
{
	for (AQPCasing* Casing : CasingPool)
	{
		if (Casing && !Casing->bIsActive)
		{
			return Casing;
		}
	}
	
	// 모든 탄피가 활성화되어 있다면 가장 앞쪽의 탄피를 강제로 재사용 (오래된 탄피)
	if (CasingPool.Num() > 0 && CasingPool[0])
	{
		AQPCasing* OldestCasing = CasingPool[0];
		CasingPool.RemoveAt(0);
		CasingPool.Add(OldestCasing); // 끝으로 보냄
		return OldestCasing;
	}
	return nullptr; 
}

void AGunWeapon::EjectCasing()
{
	// 데디케이티드 서버는 무시
	if (GetNetMode() == NM_DedicatedServer) return;
	
	if (!WeaponMesh || !WeaponMesh->DoesSocketExist(AmmoEjectSocketName)) return;

	AQPCasing* Casing = GetAvailableCasing();
	if (Casing)
	{
		FTransform SocketTransform = WeaponMesh->GetSocketTransform(AmmoEjectSocketName);
		
		// 현재 무기의 속도(캐릭터의 이동 속도 등)를 탄피에 더해줌으로써 자연스러운 물리 연출
		FVector Velocity = GetVelocity();
		if (GetOwner()) Velocity = GetOwner()->GetVelocity();

		Casing->ActivateCasing(SocketTransform, Velocity);
	}
}
