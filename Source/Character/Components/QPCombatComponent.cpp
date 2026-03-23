#include "QPCombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/HUD.h"
#include "PJ_Quiet_Protocol/UserWidget/Crosshair/QPCrosshair.h"
#include "Net/UnrealNetwork.h"

#include "TimerManager.h"
#include "Engine/World.h"

#define TRACE_LENGTH 80000.f //충돌 검사 거리

UQPCombatComponent::UQPCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	HitTarget = FVector::ZeroVector;
	TraceHitTarget = FVector::ZeroVector;
	LastHitTarget = FVector::ZeroVector;
	CrosshairScreenOffset = FVector2D::ZeroVector;

	SetIsReplicatedByDefault(true); // 컴포넌트 리플리케이션 활성화
}

void UQPCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UQPCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UQPCombatComponent, bIsAttacking);
	DOREPLIFETIME(UQPCombatComponent, bIsAiming);
	DOREPLIFETIME(UQPCombatComponent, TraceHitTarget); // 주인은 로컬에서 계산하므로 받지 않음 -> 조건 제거 (확실한 동기화)
}

void UQPCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner()); //소유한 캐릭터 가져오기

	// 게임 시작 시 크로스헤어 오프셋 초기화 (HUD에서 가져오기)
	if (OwnerCharacter)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController()))
		{
			if (AQPCrosshair* HUD = Cast<AQPCrosshair>(PlayerController->GetHUD()))
			{
				CrosshairScreenOffset = HUD->CrosshairScreenOffset;
			}
		}
	}
}

void UQPCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 로컬 플레이어만 HitTarget 계산 및 크로스헤어 위치 업데이트 수행
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// 거리 10cm 이상 차이날 때만 전송 (네트워크 최적화)
		if (FVector::DistSquared(HitTarget, LastHitTarget) > 100.f) 
		{
			// 0.05초마다 전송 (네트워크 최적화)
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastHitTargetRPCTime > 0.05f)
			{
				ServerSetHitTarget(HitTarget);
				LastHitTarget = HitTarget; // 마지막으로 전송한 HitTarget 저장
				LastHitTargetRPCTime = CurrentTime;
			}
		}

		UpdateCrosshairPosition(DeltaTime); // 크로스헤어 위치 동적 업데이트 (로컬)
	}
	else if (OwnerCharacter)
	{
		// 서버/다른 클라이언트에서는 TraceHitTarget을 그대로 사용 (동기화된 값)
		HitTarget = TraceHitTarget; 
	}
}

void UQPCombatComponent::ServerSetHitTarget_Implementation(const FVector_NetQuantize& TraceHitTarget_Arg)
{
	TraceHitTarget = TraceHitTarget_Arg; // 서버에서 받은 TraceHitTarget을 저장
	HitTarget = TraceHitTarget_Arg; // 서버에서도 HitTarget 업데이트 (서버/다른 클라이언트는 TraceHitTarget 사용)
}

void UQPCombatComponent::UpdateCrosshairPosition(float DeltaTime)
{
	if (!OwnerCharacter) return;

	// 목표 오프셋 계산
	FVector2D TargetOffset = HipFireCenterOffset;

	// 현재 Pitch 가져오기 (-90 ~ 90)
	FRotator ControlRot = OwnerCharacter->GetControlRotation();
	float Pitch = ControlRot.Pitch;
	if (Pitch > 180.f) Pitch -= 360.f; // 정규화 (-180 ~ 180)

	// 1. 조준 상태 (Aiming)
	if (bIsAiming)
	{
		TargetOffset = AimingBaseOffset;
		float VerticalShift = (Pitch / 90.f) * AimingVerticalScale; 
		TargetOffset.Y += VerticalShift;
	}
	// 2. 비조준 상태 (Hip-Fire)
	else
	{
		// Pitch < 0 (위를 봄) -> HipFireUpOffset으로 보간
		if (Pitch < 0.f)
		{
			float Alpha = FMath::Abs(Pitch) / 90.f;
			TargetOffset = FMath::Lerp(HipFireCenterOffset, HipFireUpOffset, Alpha);
		}
		// Pitch > 0 (아래를 봄) -> HipFireDownOffset으로 보간
		else
		{
			float Alpha = Pitch / 90.f;
			TargetOffset = FMath::Lerp(HipFireCenterOffset, HipFireDownOffset, Alpha);
		}
	}

	// 3. 앉기 상태 (Crouching) - 조준/비조준 공통 적용
	if (OwnerCharacter->bIsCrouched)
	{
		TargetOffset.Y += CrouchCrosshairOffset;
	}

	// 현재 오프셋에서 목표 오프셋으로 보간 (부드러운 이동)
	CrosshairScreenOffset = FMath::Vector2DInterpTo(CrosshairScreenOffset, TargetOffset, DeltaTime, 10.f);

	// HUD 업데이트 (동기화)
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (AQPCrosshair* HUD = Cast<AQPCrosshair>(PC->GetHUD()))
		{
			HUD->CrosshairScreenOffset = CrosshairScreenOffset;
		}
	}
}

void UQPCombatComponent::OnRep_EquippedWeapon() //서버에서 EquippedWeapon이 변경될 때마다 클라이언트에서 호출
{
	if (!OwnerCharacter) 
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner()); //소유한 캐릭터가 유효하지 않으면 다시 가져오기
	}
	
	if (EquippedWeapon && OwnerCharacter) 
	{
		EquippedWeapon->OnEquipped(OwnerCharacter);  //무기 장착 처리 호출
		AttachWeaponToCharacter(EquippedWeapon); //캐릭터에 무기 부착
		SetWeaponType(EquippedWeapon->GetWeaponType()); //무기 타입 설정
	}
}

bool UQPCombatComponent::EquipWeapon(AWeaponBase* NewWeapon, bool bUnequipCurrent) 
{
	if (!OwnerCharacter || !NewWeapon) return false; //소유한 캐릭터나 새 무기가 유효하지 않으면 false 반환
	if (EquippedWeapon == NewWeapon) return true; //이미 장착된 무기라면 true 반환
	if (EquippedWeapon && bUnequipCurrent) { //현재 무기가 있고 해제 플래그가 true라면
		UnEquipWeapon(true); //현재 무기 해제
	}
	EquippedWeapon = NewWeapon; //새 무기 장착

	//소유자/충돌 기본 처리 (나중에 확장)
	EquippedWeapon->SetOwner(OwnerCharacter);//소유자 설정
	EquippedWeapon->SetInstigator(Cast<APawn>(OwnerCharacter)); //인스티게이터 설정
	EquippedWeapon->SetActorEnableCollision(false); //충돌 비활성화
	EquippedWeapon->OnEquipped(OwnerCharacter); //무기 장착 처리 호출
	if (!AttachWeaponToCharacter(EquippedWeapon)) //캐릭터에 무기 부착 실패 시
	{
		EquippedWeapon->OnUnequipped(true); //무기 해제 처리 호출
		EquippedWeapon = nullptr; //장착 실패 시 무기 초기화
		SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
		return false; //false 반환
	}
	SetWeaponType(NewWeapon->GetWeaponType()); //무기 타입 설정 ( WeaponBase에서 GetWeaponType() 구현 필요 )
	return true; //성공적으로 장착했으므로 true 반환
}

bool UQPCombatComponent::UnEquipWeapon(bool bDropToWorld)
{
	if (!OwnerCharacter || !EquippedWeapon) {
		SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
		return false; //소유한 캐릭터나 장착된 무기가 없으면 false 반환
	}
	StopAttack(); //공격 중지

	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); //월드 트랜스폼 유지하며 분리
	EquippedWeapon->OnUnequipped(bDropToWorld); //무기 해제 처리 호출
	EquippedWeapon = nullptr; //장착된 무기 초기화

	SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
	return true; //성공적으로 해제했으므로 true 반환
}

//서버에서 클라이언트로 무기 장착 요청 처리 함수 구현
void UQPCombatComponent::ServerEquipWeapon_Implementation(AWeaponBase* WeaponToEquip)
{
	EquipWeapon(WeaponToEquip, true);
} 

void UQPCombatComponent::StartAttack()
{
	if (!EquippedWeapon) return;

	// 단발 무기이거나 근접 무기인 경우, 마우스를 꾹 누르고 있어도 한 번만 발사되도록 검사
	if (!EquippedWeapon->IsAutomatic() || EquippedWeapon->GetWeaponType() == EQPWeaponType::EWT_Melee)
	{
		if (!bCanFireSingleShot) return; // 이미 이번 클릭에서 한 발 쐈으면 무시
		bCanFireSingleShot = false; // 플래그 잠금
	}

	// [Fix] 클라이언트(로컬)에서도 쿨타임 검사를 수행하여 연타 시 애니메이션 재시작을 방지
	if (!CanFire(EquippedWeapon->IsAutomatic())) return;

	SetIsAttacking(true); // [Prediction] 로컬에서 즉시 반응
	ServerStartAttack(); // 서버에 공격 요청
}

//발사 가능 여부 체크 함수 (자동/비자동 무기 구분)
bool UQPCombatComponent::CanFire(bool bAutomatic)
{
	if (!EquippedWeapon) return false;
	
	// 자동 무기가 아니더라도 무기에 설정된 FireRate 만큼 일정한 쿨타임을 적용하여 연타를 막음
	double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < EquippedWeapon->GetFireRate())
	{
		return false;
	}
	return true;
}

//공격 시작 서버 함수 구현
void UQPCombatComponent::ServerStartAttack_Implementation()
{
	if (!EquippedWeapon) return;

	// 근접 무기 및 단/연발 화기를 포함하여 모두 설정된 FireRate 만큼의 쿨타임 체크 수행
	if (!CanFire(EquippedWeapon->IsAutomatic())) return;

	SetIsAttacking(true); // 공격 상태 true로 설정 (Replicated)
	
	Fire(); // 서버에서 발사 처리 (Multicast 호출)

	// 자동 무기인 경우 발사 타이머 시작 (연사 제어)
	if (EquippedWeapon->IsAutomatic()) 
	{
		StartFireTimer(); // 발사 타이머 시작
	}
}

void UQPCombatComponent::StopAttack()
{
	bCanFireSingleShot = true; // 단발 발사 플래그 초기화 (마우스 뗐을 때)

	SetIsAttacking(false); //공격 상태 false로 설정
	ServerStopAttack(); //서버에 공격 중지 요청
}

void UQPCombatComponent::ServerStopAttack_Implementation()
{
	if (EquippedWeapon) {
		EquippedWeapon->StopAttack(); //무기 공격 중지
	}
	SetIsAttacking(false); //공격 상태 false로 설정

	GetWorld()->GetTimerManager().ClearTimer(FireTimer); //발사 타이머 해제
}

void UQPCombatComponent::Reload()
{
	// 탄약 체크 등 선행 조건 확인 (추후 구현)
	if (EquippedWeapon)
	{
		ServerReload();
	}
}

void UQPCombatComponent::ServerReload_Implementation()
{
	if (!OwnerCharacter || !EquippedWeapon) return; //소유한 캐릭터나 장착된 무기가 유효하지 않으면 반환

	MulticastReload(); // 모든 클라이언트에서 재장전 애니메이션/이펙트 동기화용 함수 호출 (Unreliable로 설정하여 네트워크 최적화)
}

void UQPCombatComponent::MulticastReload_Implementation()
{
	if (AQPCharacter* QPChar = Cast<AQPCharacter>(OwnerCharacter))
	{
		QPChar->PlayReloadMontage(); //재장전 몽타주 재생 (모든 클라이언트에서)
	}
}

void UQPCombatComponent::Fire()
{
	if (OwnerCharacter && !OwnerCharacter->HasAuthority()) return; // 서버에서만 발사 처리 수행 (클라이언트는 ServerStartAttack에서 예측적으로 반응)

	if (!EquippedWeapon || !OwnerCharacter) return; //소유한 캐릭터나 장착된 무기가 유효하지 않으면 반환
	
	LastFireTime = GetWorld()->GetTimeSeconds(); //발사 시점 업데이트 (쿨타임 체크용)

	EquippedWeapon->StartFire(); //무기 발사 시작
	MulticastFire(bIsAiming); //모든 클라이언트에서 발사 애니메이션 재생 (조준 상태 전달)
}

void UQPCombatComponent::MulticastFire_Implementation(bool bInIsAiming)
{
	if (AQPCharacter* QPChar = Cast<AQPCharacter>(OwnerCharacter)) //애니메이션 재생
	{
		QPChar->PlayFireMontage(bInIsAiming); //조준 상태에 따라 발사 몽타주 재생
	}
}

void UQPCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !OwnerCharacter) return;

	GetWorld()->GetTimerManager().SetTimer(
		FireTimer,
		this,
		&UQPCombatComponent::Fire,
		EquippedWeapon->GetFireRate(),
		true //반복 재생
	);
}


bool UQPCombatComponent::AttachWeaponToCharacter(AWeaponBase* Weapon)
{
	if (!OwnerCharacter || !Weapon) return false; //소유한 캐릭터나 무기가 유효하지 않으면 false 반환
	USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh(); //캐릭터의 스켈레탈 메쉬 컴포넌트 가져오기
	if (!MeshComponent) return false; //메쉬 컴포넌트가 유효하지 않으면 false 반환
	Weapon->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, EquipSocketName); //무기를 캐릭터 메쉬에 부착
	return true; //성공적으로 부착했으므로 true 반환
}

void UQPCombatComponent::SetWeaponType(EQPWeaponType NewType)
{
	if (EquippedWeaponType == NewType) return; //이미 같은 타입이면 반환
	EquippedWeaponType = NewType; //무기 타입 설정
	OnWeaponTypeChanged.Broadcast(EquippedWeaponType); //무기 타입 변경 델리게이트 브로드캐스트
}
void UQPCombatComponent::SetIsAttacking(bool bNewIsAttacking)
{
	if (bIsAttacking == bNewIsAttacking) return; //이미 같은 상태이면 반환
	bIsAttacking = bNewIsAttacking; //공격 상태 설정
	OnAttackStateChanged.Broadcast(bIsAttacking); //공격 상태 변경 델리게이트 브로드캐스트
}

void UQPCombatComponent::SetAiming(bool bNewAiming) //조준 상태 설정 함수
{
	ServerSetAiming(bNewAiming); // 서버에 조준 상태 변경 요청
}

void UQPCombatComponent::ServerSetAiming_Implementation(bool bNewAiming)
{
	SetIsAiming(bNewAiming); // 서버에서 상태 변경 및 델리게이트 호출
}

void UQPCombatComponent::SetIsAiming(bool bNewIsAiming) 
{
	bIsAiming = bNewIsAiming; //조준 상태 설정

	OnAimStateChanged.Broadcast(bIsAiming); //조준 상태 변경 델리게이트 브로드캐스트
}


void UQPCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!OwnerCharacter) return;

	FVector2D ViewportSize = FVector2D::ZeroVector; //뷰포트 크기 변수
	if (GEngine && GEngine->GameViewport) 
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); //뷰포트 크기 가져오기
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); //화면 중앙 위치 계산
	
	// 오프셋 적용 (화면 중앙 + 오프셋)
	CrosshairLocation.X += CrosshairScreenOffset.X;
	CrosshairLocation.Y += CrosshairScreenOffset.Y;

	FVector CorsshairWorldLocation, CorsshairWorldDirection; //월드 위치 및 방향 변수
	
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()); //플레이어 컨트롤러 가져오기
	if (!PC) return; // Controller가 없으면 트레이스 불가

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		PC, //플레이어 컨트롤러 가져오기
		CrosshairLocation, //스크린 위치
		CorsshairWorldLocation, //월드 위치 출력
		CorsshairWorldDirection //월드 방향 출력
	);

	if (bScreenToWorld) //스크린을 월드로 변환 성공 시
	{
		const FVector Start = CorsshairWorldLocation; //시작 위치 설정
		const FVector End = Start + (CorsshairWorldDirection * TRACE_LENGTH); //끝 위치 설정 (80,000 유닛 앞)
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(OwnerCharacter); // 자기 자신(캐릭터) 무시하여 엉뚱한 충돌 방지
		if (EquippedWeapon)
		{
			QueryParams.AddIgnoredActor(EquippedWeapon); //장착된 무기도 트레이스에서 제외 (자신의 무기에 시야가 가려지는 현상 방지)
		}

		// 라인 트레이스 수행 (충돌 검사)
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility,
			QueryParams
		);

		// 충돌이 발생하지 않은 경우, 트레이스 끝 지점을 ImpactPoint로 설정하여 크로스헤어가 가리키는 위치를 유지
		if (!bHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}

}

FVector UQPCombatComponent::GetMuzzleHitTarget() const
{
	if (!EquippedWeapon) return FVector::ZeroVector;

	// MuzzleFlash 소켓이 존재하는지 확인하고, 해당 소켓의 위치와 방향을 기반으로 트레이스하여 충돌 지점 계산
	USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh();
	if (!WeaponMesh || !WeaponMesh->DoesSocketExist(TEXT("MuzzleFlash"))) return FVector::ZeroVector;

	const FTransform MuzzleTransform = WeaponMesh->GetSocketTransform(TEXT("MuzzleFlash"), RTS_World);
	const FVector Start = MuzzleTransform.GetLocation();
	const FVector End = Start + (MuzzleTransform.GetUnitAxis(EAxis::X) * TRACE_LENGTH);

	FHitResult FireHit; // 트레이스 결과 저장 변수
	GetWorld()->LineTraceSingleByChannel(
		FireHit,
		Start,
		End,
		ECollisionChannel::ECC_Visibility
	);

	if (FireHit.bBlockingHit) // 충돌이 발생한 경우, 충돌 지점을 반환
	{
		return FireHit.ImpactPoint;
	}
	return End;
}