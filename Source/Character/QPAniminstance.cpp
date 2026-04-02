#include "QPAnimInstance.h"
#include "QPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h" 
#include "Components/SkeletalMeshComponent.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

void UQPAniminstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 애니메이션 인스턴스를 소유한 폰(캐릭터) 가져오기
	APawn* Pawn = TryGetPawnOwner();
	CachedCharacter = Cast<AQPCharacter>(Pawn);

	// 초기 오프셋 및 Yaw 값 설정
	DefaultRightHandOffset = RightHandOffset; 
	SmoothedDeltaYaw = 0.f; 
}

void UQPAniminstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 캐릭터 참조가 유효하지 않으면 다시 가져오기 시도
	if (!CachedCharacter.IsValid())
	{
		APawn* Pawn = TryGetPawnOwner();
		CachedCharacter = Cast<AQPCharacter>(Pawn);
	}
	AQPCharacter* Character = CachedCharacter.Get();
	if (!Character) return;

	// [1] 기본 움직임 및 상태값 동기화
	Speed = Character->GetVelocity().Size2D();
	bIsCrouched = Character->bIsCrouched;
	bIsSprinting = Character->IsSprinting();
	bIsAiming = Character->IsAiming();
	bIsTurningInPlace = Character->IsTurningInPlace();

	// 사망 상태일 때 불필요한 레이축(IK, 조준 등) 비활성화
	if (Character->IsDead()) 
	{
		bUseLeftHandIK = false;
		bIsAiming = false;
		bIsAttacking = false;
		bIsTurningInPlace = false;
	}

	// [2] 이동 방향(Direction) 계산 - Blendspace에서 이동 애니메이션 결정용
	if (Speed > 0.f)
	{
		const FVector Velocity = Character->GetVelocity();
		const FRotator ActorRotation = Character->GetActorRotation();

		if (!Velocity.IsZero())
		{
			// 캐릭터의 로컬 좌표계 기준으로 속도 벡터 변환
			FVector LocalVelocity = ActorRotation.UnrotateVector(Velocity);
			float TargetDirection = LocalVelocity.Rotation().Yaw;
			FRotator CurrentRot = FRotator(0.f, Direction, 0.f);
			FRotator TargetRot = FRotator(0.f, TargetDirection, 0.f);
			// 방향 전환 시의 떨림 방지를 위해 보간(Interp) 적용
			Direction = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, 6.0f).Yaw;
		}
	}
	else
	{
		Direction = 0.f;
	}

	// [3] 무브먼트 컴포넌트 데이터 확인 (공중 부양, 가속 중 등)
	if (UCharacterMovementComponent* MoveComponent = Character->GetCharacterMovement()) 
	{
		bIsInAir = MoveComponent->IsFalling();
		bIsAccelerating = MoveComponent->GetCurrentAcceleration().SizeSquared() > 0.f;
	}
	else 
	{
		bIsInAir = false;
		bIsAccelerating = false;
	}
	
	// 정지 중인지 확인 (속도는 있으나 가속이 없는 지면 상태)
	bIsStopping = (Speed > 0.f) && !bIsAccelerating && !bIsInAir;

	// [4] 컴뱃 컴포넌트로부터 무기 및 전투 상태 가져오기
	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())
	{
		WeaponType = CombatComponent->GetEquippedWeaponType();
		bIsAttacking = CombatComponent->IsAttacking();
		bIsReloading = CombatComponent->IsReloading();
	}
	else
	{
		WeaponType = EQPWeaponType::EWT_None;
		bIsAttacking = false;
		bIsReloading = false;
	}

	// [5] 에임 오프셋(AO) 보간 처리
	AO_Yaw = Character->GetAO_Yaw();
	AO_Pitch = Character->GetAO_Pitch();

	// 로컬/리모트 컨트롤에 따른 조준 회전값 스무딩
	if (Character->IsLocallyControlled())
	{
		SmoothedControlRotation = Character->GetControlRotation();
	}
	else
	{
		// 네트워크 프록시의 경우 동기화된 값을 기반으로 보간
		FRotator TargetControlRot = FRotator(Character->GetAO_Pitch(), Character->GetNetAimYaw(), 0.f);
		SmoothedControlRotation = FMath::RInterpTo(SmoothedControlRotation, TargetControlRot, DeltaSeconds, 20.f); 
	}

	// 시뮬레이티드 프록시의 제자리 회전 및 에임 보간
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		float DiscreteDeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(SmoothedControlRotation, Character->GetActorRotation()).Yaw;
		SmoothedDeltaYaw = FMath::FInterpTo(SmoothedDeltaYaw, DiscreteDeltaYaw, DeltaSeconds, 15.f);

		AO_Yaw = FMath::Clamp(SmoothedDeltaYaw, -90.f, 90.f);
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(SmoothedControlRotation, Character->GetActorRotation());
		float Pitch = FRotator::NormalizeAxis(DeltaRot.Pitch);
		bool bIsHoldingGun = (WeaponType == EQPWeaponType::EWT_Rifle || WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun);
		AO_Pitch = bIsHoldingGun ? FMath::Clamp(Pitch, -30.f, 40.f) : FMath::Clamp(Pitch, -90.f, 90.f);
	}

	// [6] 손 IK(Inverse Kinematics) 및 무기 파지 위치 정정
	bUseLeftHandIK = false;

	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())
	{
		if (AWeaponBase* EquippedWeapon = CombatComponent->GetEquippedWeapon())
		{
			if (USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh()) 
			{
				USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
				if (CharacterMesh)
				{
					// [6-1] 무기 총구 위치 및 탄착 지점 계산
					const FTransform MuzzleTransform_World = WeaponMesh->GetSocketTransform(TEXT("MuzzleFlash"), RTS_World);
					const FTransform MeshToWorld = CharacterMesh->GetComponentTransform();
					const FTransform MuzzleTransform_CS = MuzzleTransform_World.GetRelativeTransform(MeshToWorld);
					const FVector MuzzleLocation_CS = MuzzleTransform_CS.GetLocation();
					
					const FVector HitTarget_World = CombatComponent->HitTarget;
					FVector HitTarget_CS = MeshToWorld.InverseTransformPosition(HitTarget_World);

					// 타겟이 너무 뒤에 있을 때의 최소 보정값
					if (HitTarget_CS.X < 10.f) HitTarget_CS.X = 10.f; 

					FQuat FrameDeltaQuat = FQuat::Identity; 
					
					// 스프린트 중이 아닐 때만 IK 적용
					bool bShouldApplyIK = !bIsSprinting;
					if (Character->IsLocallyControlled()) 
					{
						// 로컬에서는 히트 타겟이 유효하고 거리가 일정 이상일 때 보정
						bShouldApplyIK &= (!HitTarget_World.IsZero() && FVector::DistSquared(MuzzleLocation_CS, HitTarget_CS) > 1000.f);
					}

					// [6-2] 조준 방향 정렬(Rotation Correction) 계산
					if (bShouldApplyIK)
					{
						FVector ForwardVector = FRotationMatrix(SmoothedControlRotation).GetUnitAxis(EAxis::X);
						FVector VirtualTarget_World = Character->GetActorLocation() + (ForwardVector * 10000.f); 
						FVector VirtualTarget_CS = MeshToWorld.InverseTransformPosition(VirtualTarget_World);

						FVector ForwardDir = (VirtualTarget_CS - MuzzleLocation_CS).GetSafeNormal();
						FVector RightVector = FRotationMatrix(SmoothedControlRotation).GetUnitAxis(EAxis::Y);
						FVector RightVector_CS = MeshToWorld.InverseTransformVector(RightVector);
						FRotator TargetRotIK = UKismetMathLibrary::MakeRotFromXZ(ForwardDir, RightVector_CS);

						// 쿼터니언 보간을 통해 부드러운 회전 적용
						FQuat TargetQuat = TargetRotIK.Quaternion();
						FQuat CurrentQuat = MuzzleTransform_CS.GetRotation();
						FQuat ErrorQuat = TargetQuat * CurrentQuat.Inverse();
						
						float HandInterpSpeed = (Character->IsLocallyControlled()) ? 30.f : 5.f;
						FrameDeltaQuat = FQuat::Slerp(FQuat::Identity, ErrorQuat, FMath::Min(1.f, DeltaSeconds * HandInterpSpeed));
					}
					else
					{
						// IK 미적용 시 초기값으로 부드럽게 복귀
						const FQuat CurrentCorrection = FQuat(HandRotationCorrection);
						const FQuat TargetCorrection = FQuat::Identity;
						const FQuat NewCorrection = FQuat::Slerp(CurrentCorrection, TargetCorrection, DeltaSeconds * 5.f);
						FrameDeltaQuat = CurrentCorrection.Inverse() * NewCorrection;
					}

					// 누적된 회전값 업데이트
					FQuat CurrentAccumulated = FQuat(HandRotationCorrection);
					FQuat NewAccumulated = FrameDeltaQuat * CurrentAccumulated; 

					// [6-3] 각도 한계(Angle Clamp) 및 알파 블렌딩 적용
					float IKAlpha = 1.0f;
					if (AO_Pitch > 40.f) // 위로 너무 많이 볼 때 IK 강도 약화 (메쉬 꼬임 방지)
					{
						float BlendAlpha = FMath::GetMappedRangeValueClamped(FVector2D(40.f, 85.f), FVector2D(0.f, 1.f), AO_Pitch);
						IKAlpha = FMath::InterpEaseInOut(1.0f, 0.4f, BlendAlpha, 2.0f);
					}
					NewAccumulated = FQuat::Slerp(FQuat::Identity, NewAccumulated, IKAlpha);
					NewAccumulated.Normalize();

					float AngleValue;
					FVector AxisVector;
					NewAccumulated.ToAxisAndAngle(AxisVector, AngleValue);

					// 상하 조준 각도에 따른 회전 제한값 동적 조정
					float MaxAngleDegrees = 90.f;
					if (AO_Pitch > 0.f)
					{
						MaxAngleDegrees = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20.f), FVector2D(90.f, 80.f), AO_Pitch);
					}
					float MaxAngle = FMath::DegreesToRadians(MaxAngleDegrees);
					
					if (AngleValue > MaxAngle)
					{
						NewAccumulated = FQuat(AxisVector, MaxAngle); 
					}
					HandRotationCorrection = NewAccumulated.Rotator();

					// [6-4] 왼손 파지 위치(Left Hand Socket) IK 계산
					FName TargetSocketName = bIsReloading ? TEXT("ReloadLeftHandSocket") : TEXT("LeftHandSocket");

					if (WeaponMesh->DoesSocketExist(TargetSocketName))
					{
						FTransform SocketToWeapon = WeaponMesh->GetSocketTransform(TargetSocketName, RTS_Component);
						FTransform WeaponToParent = WeaponMesh->GetRelativeTransform();
						FTransform ParentToBone = FTransform::Identity;
						FName AttachSocketName = WeaponMesh->GetAttachSocketName();
						
						// 부착된 소켓과 손의 뼈대 사이의 상대 오프셋 계산
						if (AttachSocketName != NAME_None && AttachSocketName != TEXT("hand_r")) 
						{
							FTransform AttachSocketWorld = CharacterMesh->GetSocketTransform(AttachSocketName, RTS_World);
							FTransform HandBoneWorld = CharacterMesh->GetSocketTransform(TEXT("hand_r"), RTS_World);
							ParentToBone = AttachSocketWorld.GetRelativeTransform(HandBoneWorld);
						}

						// 최종적인 왼손 IK 타겟 트랜스폼 산출
						FTransform TargetTransform = SocketToWeapon * WeaponToParent * ParentToBone;
						LeftHandIKTransform = TargetTransform;
						bUseLeftHandIK = true;
					}
				}
			}
			else
			{
				// 무기 메쉬가 없을 경우 회전 보정값 초기화
				const FQuat CurrentCorrection = FQuat(HandRotationCorrection);
				const FQuat TargetCorrection = FQuat::Identity;
				HandRotationCorrection = FQuat::Slerp(CurrentCorrection, TargetCorrection, DeltaSeconds * 5.f).Rotator();
			}
		}
	}

	// [7] 무기 타입에 따른 상체(Spine) 및 오프셋 미세 조정
	FRotator TargetSpineRotation = FRotator::ZeroRotator;
	FVector TargetRightElbowOffset = FVector::ZeroVector;
	FVector HandAdjustment = FVector::ZeroVector;

	if (WeaponType == EQPWeaponType::EWT_Rifle || WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Handgun)
	{
		float AbsPitch = FMath::Abs(AO_Pitch);
		if (WeaponType == EQPWeaponType::EWT_Handgun)
		{
			// 권총 조준 시 조준 각도에 따른 상체 비틀림 정정
			if (AO_Pitch < 0.f)
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(5.f, 0.f), AO_Pitch); 
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f); 
			}
			else
			{
				float YawAdjustment = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 10.f), AO_Pitch);
				TargetSpineRotation = FRotator(0.f, YawAdjustment, 0.f);
			}
			TargetRightElbowOffset = FVector::ZeroVector; 
		}
		else if (WeaponType == EQPWeaponType::EWT_Shotgun || WeaponType == EQPWeaponType::EWT_Rifle)
		{
			// 장축무기 조준 시 견착 위치 및 팔꿈치 오프셋 조정
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
			// 사격 시의 연동 거리 조정
			HandAdjustment.Y = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 0.f), AbsPitch);
			HandAdjustment.X = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 15.f), AbsPitch);
		}
	}

	// 보간을 통한 부드러운 상태 변화
	SpineRotation = FMath::RInterpTo(SpineRotation, TargetSpineRotation, DeltaSeconds, 4.0f);
	RightElbowOffset = FMath::VInterpTo(RightElbowOffset, TargetRightElbowOffset, DeltaSeconds, 4.0f);

	FVector TargetRightHandOffset = DefaultRightHandOffset + HandAdjustment;
	RightHandOffset = FMath::VInterpTo(RightHandOffset, TargetRightHandOffset, DeltaSeconds, 5.0f);

	// [8] 하반신 회전 오프셋(RootYawOffset) 계산 - 제자리 회전 애니메이션 처리용
	FRotator ControlRotation = FRotator(0.f, Character->GetBaseAimRotation().Yaw, 0.f);
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		ControlRotation = FRotator(0.f, SmoothedControlRotation.Yaw, 0.f);
	}
	const FRotator ActorRotation = Character->GetActorRotation(); 
	const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation).Yaw;

	float TargetRootYawOffset = DeltaYaw - AO_Yaw;
	if (Character->GetLocalRole() == ROLE_SimulatedProxy)
	{
		TargetRootYawOffset = SmoothedDeltaYaw - AO_Yaw; 
	}

	// 정지 중일 때만 상하체 분리 회전(RootYawOffset) 유지
	if (Speed < 5.f && !bIsInAir)
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, TargetRootYawOffset, DeltaSeconds, 6.f);
	}
	else
	{
		// 이동 중에는 오프셋을 초기화하여 하체가 이동 방향을 따르도록 함
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.f, DeltaSeconds, 8.f); 
	}
	RootYawOffset = FMath::Clamp(RootYawOffset, -90.f, 90.f);
}
