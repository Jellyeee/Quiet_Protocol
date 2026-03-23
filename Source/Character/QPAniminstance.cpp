// Fill out your copyright notice in the Description page of Project Settings.


#include "QPAnimInstance.h"
#include "QPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h" 
#include "Components/SkeletalMeshComponent.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

void UQPAniminstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* Pawn = TryGetPawnOwner();
	CachedCharacter = Cast<AQPCharacter>(Pawn);

	DefaultRightHandOffset = RightHandOffset; 

	SmoothedDeltaYaw = 0.f; 
}

void UQPAniminstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!CachedCharacter.IsValid())
	{
		APawn* Pawn = TryGetPawnOwner();
		CachedCharacter = Cast<AQPCharacter>(Pawn);
	}
	AQPCharacter* Character = CachedCharacter.Get();
	if (!Character) return;
	Speed = Character->GetVelocity().Size2D(); //수평 속도 계산
	bIsCrouched = Character->bIsCrouched; //앉아있는지 여부 업데이트
	bIsSprinting = Character->IsSprinting(); //달리고 있는지 여부 업데이트
	bIsAiming = Character->IsAiming(); //조준하고 있는지 여부 업데이트
	bIsTurningInPlace = Character->IsTurningInPlace(); //제자리 회전 중인지 여부 업데이트

	if (Character->IsDead()) 
	{
		bUseLeftHandIK = false;
		bIsAiming = false;
		bIsAttacking = false;
		bIsTurningInPlace = false;
	}
	if (Speed > 0.f) //움직임 방향 계산
	{
		const FVector Velocity = Character->GetVelocity();
		const FRotator ActorRotation = Character->GetActorRotation();

		float TargetDirection = 0.f; // 이동 방향 계산: 월드 좌표계에서의 속도를 캐릭터의 로컬 좌표계로 변환하여 Yaw 각도로 계산
		if (!Velocity.IsZero()) // 속도가 0이 아닌 경우에만 방향 계산
		{
			FVector LocalVelocity = ActorRotation.UnrotateVector(Velocity); // 월드 좌표계에서의 속도를 캐릭터의 로컬 좌표계로 변환
			TargetDirection = LocalVelocity.Rotation().Yaw; // 로컬 좌표계에서의 속도의 Yaw 각도를 이동 방향으로 사용
		}

		// 보간 적용: 급격한 애니메이션 전환 방지 (-180/180 경계 처리를 위해 RInterpTo 사용)
		FRotator CurrentRot = FRotator(0.f, Direction, 0.f);
		FRotator TargetRot = FRotator(0.f, TargetDirection, 0.f); 
		
		Direction = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, 6.0f).Yaw; // 보간 속도 조정 
	}
	else // Speed가 0인 경우, 즉 정지 상태에서는 방향을 0으로 고정
	{
		Direction = 0.f;
	}

	if (UCharacterMovementComponent* MoveComponent = Character->GetCharacterMovement()) 
	{
		bIsInAir = MoveComponent->IsFalling(); //공중에 있는지 여부 업데이트
		bIsAccelerating = MoveComponent->GetCurrentAcceleration().SizeSquared() > 0.f; //가속 중인지 여부 업데이트
	}
	else 
	{
		bIsInAir = false; //공중에 있는지 여부 업데이트
		bIsAccelerating = false; //가속 중인지 여부 업데이트
	}
	// 정지 중 = 속도는 있지만 입력(가속)이 없는 상태
	bIsStopping = (Speed > 0.f) && !bIsAccelerating && !bIsInAir;
	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())
	{
		WeaponType = CombatComponent->GetEquippedWeaponType(); //장착된 무기 타입 업데이트
		bIsAttacking = CombatComponent->IsAttacking(); //공격 중인지 여부 업데이트
	}
	else
	{
		WeaponType = EQPWeaponType::EWT_None; //장착된 무기 타입 업데이트
		bIsAttacking = false; //공격 중인지 여부 업데이트
	}

	AO_Yaw = Character->GetAO_Yaw(); // 캐릭터의 AO_Yaw와 AO_Pitch는 캐릭터 클래스에서 계산되어 반환된 값을 사용
	AO_Pitch = Character->GetAO_Pitch(); // 캐릭터의 AO_Yaw와 AO_Pitch는 캐릭터 클래스에서 계산되어 반환된 값을 사용

	// ================= (조준 시 총구 방향 보정) =================
	if (Character->IsLocallyControlled()) // Local Player의 경우, Control Rotation을 직접 사용하여 SmoothedControlRotation 업데이트 (즉각적인 반응과 정확한 조준 보장)
	{
		SmoothedControlRotation = Character->GetControlRotation();
	}
	else // Remote Player의 경우, SmoothedControlRotation을 캐릭터의 AO_Pitch와 NetAimYaw를 기반으로 보간하여 업데이트 (부드러운 회전과 네트워크 지연 보정)
	{
		FRotator TargetControlRot = FRotator(Character->GetAO_Pitch(), Character->GetNetAimYaw(), 0.f);
		SmoothedControlRotation = FMath::RInterpTo(SmoothedControlRotation, TargetControlRot, DeltaSeconds, 20.f); 
	}

	// Simulated Proxy의 경우, SmoothedControlRotation과 Actor Rotation의 차이를 기반으로 AO_Yaw와 AO_Pitch 계산 (직관적인 제자리 회전과 상체 비틀림 방지)
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		float DiscreteDeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(SmoothedControlRotation, Character->GetActorRotation()).Yaw;
		
		// DeltaYaw 보간: 빠르게 반응하면서도 90도 한계점 부근에서의 떨림 제거 (15.f -> 30.f)
		SmoothedDeltaYaw = FMath::FInterpTo(SmoothedDeltaYaw, DiscreteDeltaYaw, DeltaSeconds, 15.f);

		// Smooth 값을 기준으로 AO 및 RootYaw 계산
		AO_Yaw = FMath::Clamp(SmoothedDeltaYaw, -90.f, 90.f);
		// AO_Pitch 계산 (기존 유지)
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(SmoothedControlRotation, Character->GetActorRotation());
		float Pitch = FRotator::NormalizeAxis(DeltaRot.Pitch);
		bool bIsHoldingGun = (WeaponType == EQPWeaponType::EWT_Rifle || WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun);
		AO_Pitch = bIsHoldingGun ? FMath::Clamp(Pitch, -30.f, 40.f) : FMath::Clamp(Pitch, -90.f, 90.f);
	}

	// ================= IK (왼손 보정) =================

	bUseLeftHandIK = false; // 기본적으로 IK 비활성화

	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())	// 전투 컴포넌트 가져오기
	{

		if (AWeaponBase* EquippedWeapon = CombatComponent->GetEquippedWeapon()) // 장착된 무기 가져오기
		{
			if (USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh()) 
			{
				USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
				
				if (CharacterMesh && WeaponMesh->DoesSocketExist(TEXT("MuzzleFlash"))) 
				{

					// 1. 현재 총구와 다리(Mesh) 상태 가져오기 (Character Component Space)
					const FTransform MuzzleTransform_World = WeaponMesh->GetSocketTransform(TEXT("MuzzleFlash"), RTS_World);
					const FTransform MeshToWorld = CharacterMesh->GetComponentTransform();
					const FTransform MuzzleTransform_CS = MuzzleTransform_World.GetRelativeTransform(MeshToWorld);
					const FVector MuzzleLocation_CS = MuzzleTransform_CS.GetLocation();
					
					// 2. 타겟 좌표를 Component Space로 변환
					const FVector HitTarget_World = CombatComponent->HitTarget;
					FVector HitTarget_CS = MeshToWorld.InverseTransformPosition(HitTarget_World);

					if (HitTarget_CS.X < 10.f) HitTarget_CS.X = 10.f; 

					FQuat FrameDeltaQuat = FQuat::Identity; 
					
					// 3. IK 적용 여부 결정 (로컬/리모트 분리)
					bool bShouldApplyIK = !bIsSprinting;
					if (Character->IsLocallyControlled()) 
					{
						// 로컬 플레이어의 경우, Control Rotation과 타겟 위치를 기반으로 IK 적용 여부 결정 (즉각적인 반응과 정확한 조준 보장)
						bShouldApplyIK &= (!HitTarget_World.IsZero() && FVector::DistSquared(MuzzleLocation_CS, HitTarget_CS) > 1000.f);
					}
					

					if (bShouldApplyIK) // 달리는 중이 아니고 타겟이 유효하며 총구와 타겟 사이의 거리가 충분히 멀다면, IK 보정 계산
					{
						FVector ForwardVector = FRotationMatrix(SmoothedControlRotation).GetUnitAxis(EAxis::X);
						FVector VirtualTarget_World = Character->GetActorLocation() + (ForwardVector * 10000.f); 
						FVector VirtualTarget_CS = MeshToWorld.InverseTransformPosition(VirtualTarget_World); // 타겟이 월드 공간에서 멀리 떨어져 있지만, 총구 방향을 기준으로 한 가상의 타겟 위치를 계산하여, 조준 시 손 회전이 자연스럽게 총구 방향을 향하도록 보정

						FRotator StandardLookAt = UKismetMathLibrary::FindLookAtRotation(MuzzleLocation_CS, VirtualTarget_CS); // 총구에서 가상의 타겟을 바라보는 회전 계산
						FVector ForwardDir = (VirtualTarget_CS - MuzzleLocation_CS).GetSafeNormal(); // 총구에서 가상의 타겟 방향 계산
						
						// 총구의 Forward 방향과 가상의 타겟 방향을 기준으로 한 회전 계산 (손이 총구 방향을 향하도록 보정)
						FVector RightVector = FRotationMatrix(SmoothedControlRotation).GetUnitAxis(EAxis::Y);
						FVector RightVector_CS = MeshToWorld.InverseTransformVector(RightVector);
						FRotator TargetRot = UKismetMathLibrary::MakeRotFromXZ(ForwardDir, RightVector_CS);

						FQuat TargetQuat = TargetRot.Quaternion();
						FQuat CurrentQuat = MuzzleTransform_CS.GetRotation();
						FQuat ErrorQuat = TargetQuat * CurrentQuat.Inverse();
						
						float HandInterpSpeed = (Character->IsLocallyControlled()) ? 30.f : 5.f;
						FrameDeltaQuat = FQuat::Slerp(FQuat::Identity, ErrorQuat, FMath::Min(1.f, DeltaSeconds * HandInterpSpeed));
					}
					else // 달리는 중이거나 타겟이 유효하지 않거나 총구와 타겟 사이의 거리가 충분히 멀지 않다면, 손 회전 보정을 원래대로 서서히 되돌림
					{
						const FQuat CurrentCorrection = FQuat(HandRotationCorrection);
						const FQuat TargetCorrection = FQuat::Identity;
						const FQuat NewCorrection = FQuat::Slerp(CurrentCorrection, TargetCorrection, DeltaSeconds * 5.f);
						FrameDeltaQuat = CurrentCorrection.Inverse() * NewCorrection;
					}

					// 4. 계산된 FrameDeltaQuat를 HandRotationCorrection에 누적 적용 (회전이 너무 커지는 것을 방지하기 위해 최대 각도 제한)
					FQuat CurrentAccumulated = FQuat(HandRotationCorrection);
					FQuat NewAccumulated = FrameDeltaQuat * CurrentAccumulated; 

					// AO_Pitch에 따라 IK 보정 강도 조절 (위로 볼 때는 보정 강도 감소, 아래로 볼 때는 유지)
					float IKAlpha = 1.0f;
					if (AO_Pitch > 40.f)
					{
						float BlendAlpha = FMath::GetMappedRangeValueClamped(FVector2D(40.f, 85.f), FVector2D(0.f, 1.f), AO_Pitch); // AO_Pitch가 40도에서 85도 사이일 때, BlendAlpha가 0에서 1로 증가
						IKAlpha = FMath::InterpEaseInOut(1.0f, 0.4f, BlendAlpha, 2.0f); // AO_Pitch가 40도 이상일 때, IK 보정 강도를 1.0에서 0.4로 부드럽게 감소 (위로 볼 때 보정 강도 감소)
					}
					NewAccumulated = FQuat::Slerp(FQuat::Identity, NewAccumulated, IKAlpha); // AO_Pitch에 따른 IK 보정 강도 조절 적용

					NewAccumulated.Normalize(); // 회전이 너무 커지는 것을 방지하기 위해 정규화

					float Angle;
					FVector Axis;
					NewAccumulated.ToAxisAndAngle(Axis, Angle);

					float MaxAngleDegrees = 90.f;
					if (AO_Pitch > 0.f) // 수평선 이하로 내려갈 때
					{
						// AO_Pitch가 0도에서 20도 사이일 때, MaxAngleDegrees가 90도에서 80도로 감소 (아래로 볼 때 보정 강도 유지)
						MaxAngleDegrees = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20.f), FVector2D(90.f, 80.f), AO_Pitch);
					}
					float MaxAngle = FMath::DegreesToRadians(MaxAngleDegrees); // 최대 각도를 라디안으로 변환
					
					if (Angle > MaxAngle) // 회전이 최대 각도를 초과하면, 최대 각도로 제한
					{
						NewAccumulated = FQuat(Axis, MaxAngle); 
					}
					HandRotationCorrection = NewAccumulated.Rotator(); // 디버그 드로잉 (회전 축과 각도 시각화)  
				}
				else // 총구 소켓이 없거나 캐릭터 메쉬가 없는 경우, 손 회전 보정을 원래대로 서서히 되돌림
				{
					const FQuat CurrentCorrection = FQuat(HandRotationCorrection);
					const FQuat TargetCorrection = FQuat::Identity;
					HandRotationCorrection = FQuat::Slerp(CurrentCorrection, TargetCorrection, DeltaSeconds * 5.f).Rotator();
				}

				// 5. 왼손 IK 계산: 총에 "LeftHandSocket"이 존재한다면, 왼손이 총의 해당 소켓에 위치하도록 IK 타겟 계산
				if (WeaponMesh->DoesSocketExist(TEXT("LeftHandSocket")))
				{
					FTransform SocketToWeapon = WeaponMesh->GetSocketTransform(TEXT("LeftHandSocket"), RTS_Component);
					FTransform WeaponToParent = WeaponMesh->GetRelativeTransform();
					FTransform ParentToBone = FTransform::Identity;
					FName AttachSocketName = WeaponMesh->GetAttachSocketName();
					
					// "hand_r"이 아닌 다른 소켓에 총이 붙어 있다면, 그 소켓과 "hand_r" 사이의 상대 트랜스폼을 계산하여 ParentToBone에 적용
					if (AttachSocketName != NAME_None && AttachSocketName != TEXT("hand_r")) 
					{
						FTransform AttachSocketWorld = CharacterMesh->GetSocketTransform(AttachSocketName, RTS_World);
						FTransform HandBoneWorld = CharacterMesh->GetSocketTransform(TEXT("hand_r"), RTS_World);
						ParentToBone = AttachSocketWorld.GetRelativeTransform(HandBoneWorld);
					}

					FTransform TargetTransform = SocketToWeapon * WeaponToParent * ParentToBone;
					LeftHandIKTransform = TargetTransform;

					// 디버그 드로잉
					if (UWorld* World = GetWorld()) 
					{
						
						FTransform HandWorld = CharacterMesh->GetSocketTransform(TEXT("hand_r"), RTS_World);
						FTransform IKTargetWorld = LeftHandIKTransform * HandWorld;

						FTransform LeftHandWorld = CharacterMesh->GetSocketTransform(TEXT("hand_l"), RTS_World);
					}

					bUseLeftHandIK = true;
				}
			}
		}
	}

	FRotator TargetSpineRotation = FRotator::ZeroRotator;
	FVector TargetRightElbowOffset = FVector::ZeroVector;
	FVector HandAdjustment = FVector::ZeroVector;

	// 무기 타입에 따른 상체 회전과 팔꿈치 보정, 손 위치 보정 로직
	if (WeaponType == EQPWeaponType::EWT_Rifle || WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun)
	{
		// 1. AO_Pitch의 절대값 계산 (위/아래 보는 정도에 따른 보정 강도 조절용)
		float AbsPitch = FMath::Abs(AO_Pitch);
		
		// 2. 무기 타입별 파라미터 분기
		if (WeaponType == EQPWeaponType::EWT_Handgun) // 권총 전용 로직 (상체 회전은 최소화, 팔꿈치 보정 없음)
		{
			if (AO_Pitch < 0.f) // 위를 볼 때
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(5.f, 0.f), AO_Pitch); 
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f); 
			}
			else // 아래를 볼 때
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 10.f), AO_Pitch);
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);
			}

			TargetRightElbowOffset = FVector::ZeroVector; 
		}
		else if (WeaponType == EQPWeaponType::EWT_Shotgun) //  샷건 전용 로직 (현재는 Rifle과 동일한 로직 )
		{
			if (AO_Pitch < 0.f)
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(5.f, 0.f), AO_Pitch); 
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);

				float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(1.f, 0.f), AO_Pitch);
				TargetRightElbowOffset = FMath::Lerp(FVector::ZeroVector, ElbowRetractionAmount, Alpha);
			}
			else
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 10.f), AO_Pitch);
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);

				TargetRightElbowOffset = FVector::ZeroVector;
			}

			HandAdjustment.Y = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 0.f), AbsPitch);
			HandAdjustment.X = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 15.f), AbsPitch);
		}
		else if (WeaponType == EQPWeaponType::EWT_Rifle) // [소총 전용 로직 (상체 회전과 팔꿈치 보정 모두 적용)
		{
			if (AO_Pitch < 0.f)
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(5.f, 0.f), AO_Pitch);
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);

				float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(1.f, 0.f), AO_Pitch);
				TargetRightElbowOffset = FMath::Lerp(FVector::ZeroVector, ElbowRetractionAmount, Alpha);
			}
			else
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 10.f), AO_Pitch);
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);

				TargetRightElbowOffset = FVector::ZeroVector;
			}

			HandAdjustment.Y = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 0.f), AbsPitch);
			HandAdjustment.X = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 15.f), AbsPitch);
		}
	}

	// 3. 계산된 목표값을 현재값에 보간 적용하여 부드러운 애니메이션 전환 구현
	SpineRotation = FMath::RInterpTo(SpineRotation, TargetSpineRotation, DeltaSeconds, 4.0f);
	RightElbowOffset = FMath::VInterpTo(RightElbowOffset, TargetRightElbowOffset, DeltaSeconds, 4.0f);

	// 손 위치 보정은 팔꿈치 보정과 상체 회전의 영향을 모두 받으므로, HandAdjustment에 팔꿈치 보정과 상체 회전에서 유도된 보정값을 누적하여 적용
	FVector TargetRightHandOffset = DefaultRightHandOffset + HandAdjustment;
	
	// 달리는 중에는 손 위치 보정을 원래대로 서서히 되돌림
	RightHandOffset = FMath::VInterpTo(RightHandOffset, TargetRightHandOffset, DeltaSeconds, 5.0f);

	// ================= Turn In Place (제자리 회전) =================

	// 1. 캐릭터의 현재 컨트롤 회전과 액터 회전 간의 Yaw 차이를 계산하여 목표 RootYawOffset을 구함
	FRotator ControlRotation = FRotator(0.f, Character->GetBaseAimRotation().Yaw, 0.f);

	// [Network] Simulated Proxy는 ControlRotation이 없으므로, 위에서 보간된 SmoothedControlRotation 사용
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// [Fix] Raw NetAimYaw 대신 Smoothed 값 사용 (상체 AO_Yaw와의 동기화 및 튐 방지)
		ControlRotation = FRotator(0.f, SmoothedControlRotation.Yaw, 0.f);
	}
	const FRotator ActorRotation = Character->GetActorRotation(); 

	// DeltaRotator를 사용하여 ControlRotation과 ActorRotation 간의 Yaw 차이를 -180도에서 180도 범위로 정규화하여 계산
	const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation,ActorRotation).Yaw;

	// 목표 하체 회전량
	float TargetRootYawOffset = DeltaYaw - AO_Yaw;
	
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)  // 상체 AO_Yaw와의 동기화 및 튐 방지
	{
		TargetRootYawOffset = SmoothedDeltaYaw - AO_Yaw; 
	}

	if (Speed < 5.f && !bIsInAir) // 정지 상태에서만 RootYawOffset이 TargetRootYawOffset을 향해 보간 적용되어 빠르게 회전하는 느낌을 줌
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, TargetRootYawOffset, DeltaSeconds, 6.f);  // 제자리 회전 중일 때는 RootYawOffset이 TargetRootYawOffset을 향해 빠르게 보간 적용되어 회전하는 느낌을 줌
	}
	else // 이동 중이거나 공중에 있을 때는 RootYawOffset이 천천히 0으로 복귀하도록 함
	{
		// 천천히 0으로 복귀
		RootYawOffset = FMath::FInterpTo(RootYawOffset,0.f,DeltaSeconds,8.f); 
	}

	RootYawOffset = FMath::Clamp(RootYawOffset, -90.f, 90.f); // RootYawOffset를 -90도에서 90도 사이로 제한하여 과도한 회전을 방지
	
}
