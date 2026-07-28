#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeGenerator.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeDoor.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "PJ_Quiet_Protocol/Weapons/GunWeapon.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "PJ_Quiet_Protocol/Character/Controllers/QPPlayerController.h" 
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h" 
#include "PJ_Quiet_Protocol/Character/QPAniminstance.h"
#include "Camera/PlayerCameraManager.h"
#include "PJ_Quiet_Protocol/Character/Components/QPStatusComponent.h"
#include "PJ_Quiet_Protocol/GameMode/QPGameMode.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "PJ_Quiet_Protocol/UserWidget/Crosshair/QPCrosshair.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/InventoryItem.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Blueprint/UserWidget.h"
#include "PJ_Quiet_Protocol/UI/QPGeneratorWidget.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"
#include "PJ_Quiet_Protocol/UI/QPKeypadWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Engine/DamageEvents.h"

AQPCharacter::AQPCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false; //컨트롤러의 Yaw 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationPitch = false; //컨트롤러의 Pitch 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationRoll = false; //컨트롤러의 Roll 회전에 따라 캐릭터 회전 안함

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); 
	if (ensure(MoveComponent)) 
	{
		// 기본 회전 설정: 이동 방향으로 자동 회전하지 않고 컨트롤러/조준에 따라 수동 제어
		MoveComponent->bOrientRotationToMovement = false; 
		MoveComponent->bUseControllerDesiredRotation = false; 
		
		// 이동 관련 기본 물리 및 속도 설정
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; 
		MoveComponent->MaxWalkSpeed = WalkSpeed; 
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; 
		MoveComponent->BrakingDecelerationWalking = 100.f; // 관성 있는 부드러운 멈춤을 위한 감속도 설정
		MoveComponent->GroundFriction = 2.f; // 지면 마찰력을 낮춰 부드러운 움직임 유도

		// [Network] 네트워크 이동 보간 최적화 (Jittering 방지)
		MoveComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}
	//카메라 붐 설정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); //스프링암 컴포넌트 생성
	CameraBoom->SetupAttachment(RootComponent); //루트 컴포넌트에 부착
	CameraBoom->TargetArmLength = CameraArmLength; //카메라와 캐릭터 사이 거리 설정
	CameraBoom->bUsePawnControlRotation = true; //컨트롤러 회전에 따라 카메라 회전 설정
	CameraBoom->bEnableCameraLag = true; //카메라 지연 활성화
	CameraBoom->CameraLagSpeed = 10.f; //카메라 지연 속도 설정

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); //카메라 컴포넌트 생성
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); //카메라 붐에 부착
	FollowCamera->bUsePawnControlRotation = false; //카메라가 폰의 회전에 따라 회전하지 않도록 설정

	//컴뱃 컴포넌트
	CombatComponent = CreateDefaultSubobject<UQPCombatComponent>(TEXT("CombatComponent")); //전투 컴포넌트 생성

	//상태 컴포넌트
	StatusComponent = CreateDefaultSubobject<UQPStatusComponent>(TEXT("StatusComponent")); //상태 컴포넌트 생성

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent")); //인벤토리 컴포넌트 생성

	NoiseEmitterComponent = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("NoiseEmitterComponent")); //노이즈 발생 컴포넌트 추가

	SetNetUpdateFrequency(100.f); // [Network] 업데이트 빈도 상향 (66 -> 100)
	SetMinNetUpdateFrequency(66.f); // [Network] 최소 빈도 상향 (33 -> 66)

	// [Network] 먼 거리에서도 애니메이션 생략 없이 갱신 (부드러운 동작 보장)
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// [Escape] 탈출 비밀번호 조각 배열 초기화 (나중에 BeginPlay에서 실제 크기 재조정)
	CollectedPassword.Empty();
}

void AQPCharacter::SetOverlappingWorldItem(AWorldItemActor* WorldItem)
{
	if (OverlappingWorldItem == WorldItem)
	{
		UpdatePickupWidgetTarget();
		return;
	}

	OverlappingWorldItem = WorldItem;
	UpdatePickupWidgetTarget();
}

void AQPCharacter::QuickLootItem(AActor* TargetActor)
{
	if (!TargetActor || !InventoryComponent) return;

	if (AWeaponBase* WeaponToLoot = Cast<AWeaponBase>(TargetActor))
	{
		UItemDataAsset* WeaponData = WeaponToLoot->GetWeaponItemData();
		if (WeaponData)
		{
			// [Server Authoritative] 클라이언트 측 추가 제거
			if (HasAuthority())
			{
				if (InventoryComponent->AddItem(WeaponData, 1))
				{
					WeaponToLoot->Destroy();
				}
			}
			else
			{
				ServerDestroyPickupActor(WeaponToLoot);
			}
		}
	}
	else if (AWorldItemActor* ItemToLoot = Cast<AWorldItemActor>(TargetActor))
	{
		UItemDataAsset* ItemData = ItemToLoot->ItemData;
		if (ItemData && ItemToLoot->Quantity > 0)
		{
			int32 FoundSlot = -1;
			int32 FoundCode = -1;

			// 1. 비밀번호 정보가 있는지 먼저 확인
			if (ItemToLoot->AssignedSlotIndex >= 1)
			{
				FoundSlot = ItemToLoot->AssignedSlotIndex;
				FoundCode = ItemToLoot->AssignedCodeNumber;

				// [로컬] 즉각적인 UI 반영을 위해 로컬 배열 먼저 업데이트
				if (AQPEscapeGameState* GS = GetWorld()->GetGameState<AQPEscapeGameState>())
				{
					if (GS->TotalGenerators > 0 && CollectedPassword.Num() != GS->TotalGenerators)
					{
						CollectedPassword.Init(-1, GS->TotalGenerators);
					}
				}

				int32 Idx = FoundSlot - 1;
				if (CollectedPassword.IsValidIndex(Idx))
				{
					CollectedPassword[Idx] = FoundCode;
				}
			}

			// 2. 인벤토리에 아이템 추가 시도
			{
				// [Server Authoritative] 클라이언트에서는 추가하지 않고 서버 RPC만 호출하여 중복 추가 방지
				ServerDestroyPickupActor(ItemToLoot, FoundSlot, FoundCode);
			}
		}
	}
}

void AQPCharacter::EquipInventoryItemAt(const FIntPoint& Cell)
{
	if (!InventoryComponent || !StatusComponent || !CombatComponent) return;

	FInventorySlot Slot;
	if (!InventoryComponent->FindSlotContaining(Cell, Slot)) return;
	if (!Slot.Item.ItemData) return;

	UItemDataAsset* Data = Slot.Item.ItemData;
	FString ItemNameStr = Data->ItemName.ToString();

	// 1. 방어복 아이템 처리 (보호막 충전)
	if (ItemNameStr.Contains(TEXT("방어복")) || ItemNameStr.Contains(TEXT("Amor")) || ItemNameStr.Contains(TEXT("Armor")))
	{
		if (HasAuthority())
		{
			StatusComponent->AddShield(Data->ShieldAmount);
			InventoryComponent->RemoveItemAt(Slot.Position);
		}
		else
		{
			// [SERVER-SYNC] 클라이언트에서 직접 제거하지 않고 서버 RPC만 호출
			// 서버에서 인벤토리 제거와 보호막 추가를 원자적으로 처리하도록 함
			ServerUpdateShield(Data->ShieldAmount, Slot.Position); 
		}
		return;
	}

	// 2. 체력 회복 아이템 처리
	if (ItemNameStr.Contains(TEXT("회복")) || ItemNameStr.Contains(TEXT("Heal")) || ItemNameStr.Contains(TEXT("구급상자")) || ItemNameStr.Contains(TEXT("Medikit")))
	{
		if (HasAuthority())
		{
			StatusComponent->AddHealth(Data->HealAmount);
			InventoryComponent->RemoveItemAt(Slot.Position);
		}
		else
		{
			ServerUpdateHealth(Data->HealAmount, Slot.Position); 
		}
		return;
	}

	// 3. 무기 아이템만 장착 처리
	if (Data->ItemType != EItemType::EIT_Weapon)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[경고] 이 아이템의 Data Asset에서 ItemType이 Weapon이 아닙니다!"));
		return;
	}
	
	if (!Data->WeaponClass)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[경고] 이 아이템의 Data Asset에서 'Weapon Class' 항목이 비어있습니다! BP_Shotgun 등을 할당하세요!"));
		return;
	}

	if (!HasAuthority())
	{
		// [Network Fix] 클라이언트는 직접 인벤토리 아이템을 지우지 않고 서버 RPC를 호출하여
		// 서버에서 슬롯 아이템 제거 및 무기 스폰/장착이 동기화되도록 함 (아이템 복사 버그 방지)
		ServerEquipInventoryItem(Cell);
		return;
	}

	if (AWeaponBase* CurWeapon = CombatComponent->GetEquippedWeapon())
	{
		int32 AmmoToSave = -1;
		if (AGunWeapon* Gun = Cast<AGunWeapon>(CurWeapon))
		{
			AmmoToSave = Gun->GetCurrentAmmo();
		}
		if (CurWeapon->GetWeaponItemData())
		{
			InventoryComponent->AddItem(CurWeapon->GetWeaponItemData(), 1, -1, -1, AmmoToSave);
		}
		CombatComponent->UnEquipWeapon(true);
		CurWeapon->Destroy(); // 월드에서 완전히 삭제하고 인벤에 넣음
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(Data->WeaponClass, Params);
	if (!NewWeapon) return;
	
	if (AGunWeapon* GunWeapon = Cast<AGunWeapon>(NewWeapon))
	{
		if (Slot.Item.CurrentAmmo != -1)
		{
			GunWeapon->SetCurrentAmmo(Slot.Item.CurrentAmmo);
		}
	}

	if (CombatComponent->EquipWeapon(NewWeapon, false))
	{
		InventoryComponent->RemoveItemAt(Slot.Position);
	}
	else
	{
		NewWeapon->Destroy();
	}
}

void AQPCharacter::ServerEquipInventoryItem_Implementation(FIntPoint ItemPos)
{
	EquipInventoryItemAt(ItemPos);
}

void AQPCharacter::DropInventoryItemAt(const FIntPoint& Cell)
{
	if (!InventoryComponent) return; // 인벤 컴포넌트가 없으면 종료

	FInventorySlot Slot; // 슬롯 데이터
	if (!InventoryComponent->FindSlotContaining(Cell, Slot)) return; // 해당 셀에 아이템이 없으면 종료
	if (!Slot.Item.ItemData) return; // 아이템 데이터가 없으면 종료

	// 1. 키카드 리셋 로직 (AssignedSlotIndex가 있으면 플레이어 수집 배열에서 제거)
	if (Slot.Item.AssignedSlotIndex > 0)
	{
		int32 Idx = Slot.Item.AssignedSlotIndex - 1;
		if (CollectedPassword.IsValidIndex(Idx))
		{
			CollectedPassword[Idx] = -1; // 비밀번호 정보 리셋
			if (HasAuthority() || GetNetMode() == NM_ListenServer) OnRep_CollectedPassword();
		}
	}

	UItemDataAsset* ItemData = Slot.Item.ItemData;
	const int32 Quantity = Slot.Item.Quantity;
	FVector DropLoc = GetActorLocation() + GetActorForwardVector() * 150.f; // 기본 드랍 위치는 캐릭터 앞
	//가끔 마우스 위치로 드랍되던 버그 수정
	const float HalfHeight = (GetCapsuleComponent()) ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f; //캡슐 반높이 가져오기
	const FVector TraceStart = DropLoc + FVector(0.f, 0.f, HalfHeight); //트레이스 시작 위치
	const FVector TraceEnd = DropLoc - FVector(0.f, 0.f, HalfHeight + 2.5f); //트레이스 끝 위치

	FHitResult GroundHit; //지면 히트 결과
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InventoryDrop), false, this); //충돌 쿼리 파라미터 설정
	Params.AddIgnoredActor(this); //자기 자신 무시

	if (UWorld* World = GetWorld()) //월드 가져오기
	{
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params)) //라인 트레이스 수행
		{
			DropLoc = GroundHit.Location + FVector(0.f, 0.f, 20.f); //히트 위치 위로 약간 올려서 드랍
		}
		else //트레이스 실패 시 기본 위치 사용
		{
			DropLoc = TraceStart; //기본 드랍 위치 설정
		}
	}

	if (HasAuthority())
	{
		ServerDropItem_Implementation(Cell, DropLoc);
	}
	else
	{
		ServerDropItem(Cell, DropLoc);
	}
}

void AQPCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 맵의 발전기 수를 세거나 GameState를 확인하여 비밀번호 자리수 파악 시도
	int32 RequiredCount = 0;
	if (AQPEscapeGameState* GS = GetWorld()->GetGameState<AQPEscapeGameState>())
	{
		RequiredCount = GS->TotalGenerators;
	}

	if (RequiredCount <= 0)
	{
		TArray<AActor*> FoundGenerators;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQPEscapeGenerator::StaticClass(), FoundGenerators);
		RequiredCount = FoundGenerators.Num();
	}

	// 탈출 비밀번호 배열 초기화 (정보가 아직 부족하면 0으로 시작하고 아이템 획득 시 재차 초기화)
	if (RequiredCount > 0)
	{
		CollectedPassword.Init(-1, RequiredCount);
	}
	else
	{
		CollectedPassword.Empty();
	}

	if (CombatComponent)
	{
		CombatComponent->OnWeaponTypeChanged.AddDynamic(this, &AQPCharacter::HandleWeaponTypeChanged); //무기 타입 변경 이벤트 바인딩
		HandleWeaponTypeChanged(CombatComponent->GetEquippedWeaponType()); //현재 무기 타입 처리

		// [Fix] 조준 상태 변경 시 속도 업데이트 (Rubber Banding 방지)
		CombatComponent->OnAimStateChanged.AddDynamic(this, &AQPCharacter::HandleAimStateChanged);
	}
	if (CameraBoom)
	{
		DefaultArmLength = CameraArmLength; //기본 카메라 거리 저장
		CameraBoom->TargetArmLength = CameraArmLength; //카메라와 캐릭터 사이 거리 설정
		CameraBoom->SocketOffset = StandingCameraOffset; //서있을 때 카메라 오프셋 설정 (TargetOffset -> SocketOffset)
	}
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (MoveComponent)
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉기 속도 설정
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; //앉기 가능 설정
	}

	UpdateMovementSpeed(); //움직임 속도 업데이트

	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView; //기본 FOV 저장
	}
	
	// 카메라 상하 회전 각도 제한
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)	
		{
			bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
			PlayerController->PlayerCameraManager->ViewPitchMin = bIsHoldingGun ? -30.f : -60.f; // 위를 보는 한계 (올려다봄)
			PlayerController->PlayerCameraManager->ViewPitchMax = bIsHoldingGun ? 40.f : 60.f;  // 아래를 보는 한계 (내려다봄) - 머리 위까지만
		}
	}
}

void AQPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 카메라 줌, 피벗, 오프셋 변경 (조준, 앉기, 피치 각도에 따른 동적 변화)
	UpdateCameraDynamics(DeltaTime);

	// 2. 사망 시 카메라 화면 연출 (캐릭터를 위에서 아래로 내려다보는 뷰로 부드럽게 전환)
	UpdateDeathCamera(DeltaTime);

	// 3. 이동 및 무기에 따른 캐릭터 지향(Rotation) 방식 결정 로직
	UpdateRotationMode();

	// 4. 에임 오프셋 (조준 방향에 따른 상체 회전 보간) 및 제자리 회전(Turn In Place) 처리
	AimOffset(DeltaTime); 

	/** 주변 상호작용 가능한 아이템/무기 위젯 타겟 갱신 */
	UpdatePickupWidgetTarget();
	
	/** 캐릭터 속도와 상태에 따라 주기적으로 AI가 들을 수 있는 발소리 노이즈 발생 */
	UpdateFootstepNoise(DeltaTime); 

	if (HasAuthority() || IsLocallyControlled())
	{
		// 추가적인 상태 업데이트가 필요한 경우 여기에 작성
	}
}

void AQPCharacter::UpdateCameraDynamics(float DeltaTime)
{
	if (IsDead() || !CameraBoom || CrouchCameraInterpSpeed <= 0.f) return;

	FVector DesiredOffset = GetDesiredCameraOffset(); // 현재 자세에 따른 원하는 카메라 오프셋 계산 (서있을 때 vs 앉아있을 때)
	float TargetArmLength = DefaultArmLength; // 기본 거리 (조준 여부와 관계없이 총을 들었을 때 적용)
	FVector TargetOffset = FVector::ZeroVector; // 카메라 피벗 위치 조정 (기본값은 캐릭터 위치)
	FRotator TargetRelRot = FRotator::ZeroRotator; // 카메라 상대 회전 (Pitch 조정용, 기본값은 회전 없음)

	// '총'을 들고 있는 경우 (조준 여부 무관하게 역동적인 드론 뷰 적용)
	bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
	if (bIsHoldingGun) 
	{
		/** 조준 여부에 따라 기본 카메라 거리를 조절하고, 총기 파지 시에는 약간 더 줌인하여 긴박감 부여 */
		float BaseArmLength = (IsAiming() ? AimingArmLength : DefaultArmLength) - 50.f; 
		TargetArmLength = BaseArmLength; 

		FRotator ControlRot = GetControlRotation();
		float Pitch = ControlRot.Pitch;
		if (Pitch > 180.f) // UE4 Pitch 정규화 (0~360 -> -180~180)
			Pitch -= 360.f;

		/** 
		 * 아래를 내려다볼 때 (Pitch < 0) 카메라가 바닥에 파묻히지 않도록 피벗을 위로 올리고, 
		 * 위에서 아래로 내려다보는 각도를 커스텀하게 조절하여 탑다운 스타일의 뷰를 제공합니다.
		 */
		if (Pitch < 0.f)  
		{
			float AbsPitch = FMath::Abs(Pitch);

			// 아래를 볼수록 캐릭터가 화면 하단에 위치하도록 피벗 높이(Z)를 최대 150cm 상승
			float AddedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 150.f), AbsPitch);
			TargetOffset.Z = AddedHeight;

			// 카메라 자체의 로컬 회전을 추가하여 내려다보는 느낌 강조 (최대 -35도)
			float AddedPitch = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, -35.f), AbsPitch);
			TargetRelRot.Pitch = AddedPitch;

			// 아래를 볼수록 캐릭터에 더 밀착되게 줌인 처리
			TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(BaseArmLength, BaseArmLength - 100.f), AbsPitch);
		}
	}
	else // 근접 무기 또는 맨손일 때의 동적 카메라 로직
	{
		if (IsAiming()) // 조준 중일 때는 Pitch에 따른 줌 대신 고정된 AimingArmLength 사용
		{
			TargetArmLength = AimingArmLength; 
		}
		else // 조준하지 않을 때는 Pitch에 따라 동적으로 줌인/줌아웃 적용
		{
			FRotator ControlRot = GetControlRotation();
			float Pitch = ControlRot.Pitch;
			if (Pitch > 180.f) // UE4의 Pitch는 0 ~ 360 범위이므로, -180 ~ 180 범위로 정규화
				Pitch -= 360.f;

			if (Pitch < 0.f) // 아래를 볼 때는 줌인 (카메라와 캐릭터 사이 거리 감소)
			{
				TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(MinVerticalArmLength, DefaultArmLength), Pitch);
			}
			else // 위를 볼 때는 줌아웃 (카메라와 캐릭터 사이 거리 증가)
			{
				TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(DefaultArmLength, MaxVerticalArmLength), Pitch);
			}
		}
	}
		
	// 카메라 위치와 회전을 부드럽게 보간하여 적용
	const FVector NewTargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, TargetOffset, DeltaTime, CrouchCameraInterpSpeed);
	CameraBoom->TargetOffset = NewTargetOffset;
		
	// SocketOffset는 카메라의 실제 위치에 영향을 주는 요소이므로, TargetOffset과 함께 보간하여 적용
	const FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DesiredOffset, DeltaTime, CrouchCameraInterpSpeed);
	CameraBoom->SocketOffset = NewSocketOffset;

	// 카메라 회전 보간 (Pitch 조정용)
	if (FollowCamera)
	{
		FRotator NewRelRot = FMath::RInterpTo(FollowCamera->GetRelativeRotation(), TargetRelRot, DeltaTime, CrouchCameraInterpSpeed);
		FollowCamera->SetRelativeRotation(NewRelRot);

		// 달리기 시 FOV 확대, 멈출 때 원래대로 보간하여 적용
		float TargetFOV = (bWantsToSprint && IsSprinting()) ? SprintFOV : DefaultFOV;
		float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, SprintFOVInterpSpeed);
		FollowCamera->SetFieldOfView(NewFOV);
	}

	const float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, CrouchCameraInterpSpeed); // 카메라 거리 보간
	CameraBoom->TargetArmLength = NewArmLength; // 카메라 거리 업데이트
}

void AQPCharacter::UpdateFootstepNoise(float DeltaTime)
{
	if (!HasAuthority() && !IsLocallyControlled()) return; // 로컬/서버 아니면 실행 무시

	float Speed = GetVelocity().Size2D();
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	// 땅 위에 있고 앉은 상태가 아닐 때 걷거나 뛰면 발생
	if (Speed > 10.f && MoveComp && MoveComp->IsMovingOnGround() && !bIsCrouched)
	{
		FootstepNoiseTimer -= DeltaTime;
		if (FootstepNoiseTimer <= 0.f)
		{
			// bWantsToSprint와 현재 속도가 걷기 이상인지 확인해서 뜀 상태 체크
			bool bIsRunning = (Speed > WalkSpeed + 50.f) || bWantsToSprint;
			// 기존(뛰기 1.0, 걷기 0.5) 대비 소음 감소 처리
			float Loudness = bIsRunning ? 0.5f : 0.2f; 
			float NextTime = bIsRunning ? 0.3f : 0.5f; // 달리면 더 빠른 주기로 소리 발생

			if (GetWorld())
			{
				UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), Loudness, this, 0.f, TEXT("FootstepNoise"));
			}

			FootstepNoiseTimer = NextTime; // 타이머 초기화 (다음 발소리까지의 시간)
		}
	}
	else
	{
		// 안 움직이거나 앉아있으면 타이머 리셋
		FootstepNoiseTimer = 0.f;
	}
}

void AQPCharacter::UpdateDeathCamera(float DeltaTime)
{
	if (!IsDead() || !bIsDeathCameraTransitioning || !CameraBoom) return;

	// 카메라 위치(거리)와 회전 보간
	float InterpSpeed = 2.0f;
	float CurrentArmLength = CameraBoom->TargetArmLength;
	FRotator CurrentRotation = CameraBoom->GetComponentRotation();

	float NewArmLength = FMath::FInterpTo(CurrentArmLength, DeathCameraTargetArmLength, DeltaTime, InterpSpeed);
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DeathCameraTargetRotation, DeltaTime, InterpSpeed);
		
	CameraBoom->TargetArmLength = NewArmLength;
	CameraBoom->SetWorldRotation(NewRotation);

	// 목표치에 거의 도달했으면 전환 종료 및 자유 시점 허용
	if (FMath::IsNearlyEqual(NewArmLength, DeathCameraTargetArmLength, 10.f) && 
		NewRotation.Equals(DeathCameraTargetRotation, 2.f))
	{
		bIsDeathCameraTransitioning = false;
		bIsDeathCameraFreeMode = true; // 자유 시점 모드 활성화

		// 회전 중심(피벗)을 기존 캐릭터 위치에서 현재 카메라 위치로 당겨옴
		// 이제 카메라는 캐릭터를 공전하지 않고, 자신의 위치에서 독립적으로 제자리 회전/이동함
		if (FollowCamera)
		{
			CameraBoom->SetWorldLocation(FollowCamera->GetComponentLocation());
			CameraBoom->TargetArmLength = 0.f;
			CameraBoom->SocketOffset = FVector::ZeroVector;
			CameraBoom->TargetOffset = FVector::ZeroVector;
		}

		// 카메라가 이제 플레이어의 회전에 영향을 받도록 설정하여, 사망 후에도 플레이어가 시점을 자유롭게 조절할 수 있도록 함
		CameraBoom->bUsePawnControlRotation = true;
		CameraBoom->bInheritPitch = true;
		CameraBoom->bInheritRoll = true;
		CameraBoom->bInheritYaw = true;

		// 현재 보고 있는 방향으로 컨트롤러 회전 동기화
		if (AController* PC = GetController())
		{
			PC->SetControlRotation(NewRotation);

			// 카메라 회전 제한(PitchMin/Max) 풀기
			if (APlayerController* PlayerController = Cast<APlayerController>(PC))
			{
				if (PlayerController->PlayerCameraManager)
				{
					PlayerController->PlayerCameraManager->ViewPitchMin = -89.9f;
					PlayerController->PlayerCameraManager->ViewPitchMax = 89.9f;
				}
			}
		}
	}
}

void AQPCharacter::UpdateRotationMode()
{
	// 기존 수동 회전 제어를 무시하고 엔진의 내장 회전을 강제 세팅했던 로직 제거 (버그 원인)
}

void AQPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent); //입력 컴포넌트 유효성 검사
	PlayerInputComponent->BindAxis("MoveForward", this, &AQPCharacter::MoveForward); //앞뒤 이동 바인딩
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AQPCharacter::MoveRight); //좌우 이동 바인딩
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AQPCharacter::Turn); //좌우 회전 바인딩
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AQPCharacter::LookUp); //상하 회전 바인딩
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AQPCharacter::StartJump); //점프 바인딩
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AQPCharacter::StopJump); //점프 멈춤 바인딩
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AQPCharacter::ToggleCrouch); //앉기/일어서기 토글 바인딩
	// Sprint (Hold)
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AQPCharacter::StartSprint); //달리기 시작 바인딩
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AQPCharacter::StopSprint); //달리기 멈춤 바인딩
	
	// Reload
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AQPCharacter::ReloadButtonPressed);

	// input 추가
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AQPCharacter::AttackPressed); //발사 바인딩
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Released, this, &AQPCharacter::AttackReleased); //발사 멈춤 바인딩
	
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &AQPCharacter::EquipPressed); //장착 바인딩
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Released, this, &AQPCharacter::EquipReleased); //장착 해제 바인딩

	PlayerInputComponent->BindAction(TEXT("Drop"), IE_Pressed, this, &AQPCharacter::DropPressed); //드랍 바인딩
	PlayerInputComponent->BindAction(TEXT("Drop"), IE_Released, this, &AQPCharacter::DropReleased); //드랍 해제 바인딩

	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AQPCharacter::AimButtonPressed); //조준 바인딩
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AQPCharacter::AimButtonReleased); //조준 멈춤 바인딩

	// Interact (F key)
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AQPCharacter::InteractPressed);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Released, this, &AQPCharacter::InteractReleased);

	// Quick Equip Slots 1~5
	PlayerInputComponent->BindAction(TEXT("EquipSlot1"), IE_Pressed, this, &AQPCharacter::EquipSlot1);
	PlayerInputComponent->BindAction(TEXT("EquipSlot2"), IE_Pressed, this, &AQPCharacter::EquipSlot2);
	PlayerInputComponent->BindAction(TEXT("EquipSlot3"), IE_Pressed, this, &AQPCharacter::EquipSlot3);
	PlayerInputComponent->BindAction(TEXT("EquipSlot4"), IE_Pressed, this, &AQPCharacter::EquipSlot4);
	PlayerInputComponent->BindAction(TEXT("EquipSlot5"), IE_Pressed, this, &AQPCharacter::EquipSlot5);
	
	// Holster Weapon (X)
	PlayerInputComponent->BindAction(TEXT("Holster"), IE_Pressed, this, &AQPCharacter::HolsterWeapon);
}
void AQPCharacter::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	if (OverlappingWeapon == Weapon) return; //겹쳐진 무기가 이미 설정된 무기와 같으면 함수 종료
	OverlappingWeapon = Weapon; //겹쳐진 무기 설정
	if (!IsLocallyControlled()) return; //로컬에서 제어되지 않는 경우 함수 종료
	if (AQPPlayerController* PlayerController = Cast<AQPPlayerController>(GetController())) //플레이어 컨트롤러 가져오기
	{
		PlayerController->SetPickupTarget(OverlappingWeapon); //픽업 타겟 설정
	}
}

void AQPCharacter::HandleWeaponTypeChanged(EQPWeaponType NewWeaponType)
{
	Weapontype = NewWeaponType; //장착된 무기 타입 업데이트1

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)	
		{
			bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
			PlayerController->PlayerCameraManager->ViewPitchMin = bIsHoldingGun ? -30.f : -60.f; // 위를 보는 한계 (올려다봄)
			PlayerController->PlayerCameraManager->ViewPitchMax = bIsHoldingGun ? 40.f : 60.f;  // 아래를 보는 한계 (내려다봄) - 머리 위까지만
		}
	}

	UpdateMovementSpeed(); // [Add] 무기 변경 시 이동 속도 즉시 갱신
}
//움직임 함수들
void AQPCharacter::MoveForward(float Value) //앞뒤 이동
{
	MoveInputVector.X = Value; // 항상 전 후 입력값 갱신 (리턴 전에 갱신해야 키에서 손을 뗐을 때 0으로 리셋됨)

	if (!Controller || Value == 0.f) return; //컨트롤러가 없거나 입력이 없으면

	if (bIsDeathCameraFreeMode)
	{
		HandleDeathCameraInput(FollowCamera ? FollowCamera->GetForwardVector() : FVector::ZeroVector, Value);
		return;
	}

	if (Value != 0.f && CurrentRepairingGenerator)
	{
		InteractReleased(); // 모든 종류의 이동 발생 시 수리 중단
	}

	UpdateMovementSpeed(); //방향이 바뀌면 속도 재계산

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); //컨트롤러의 Yaw 회전 가져오기
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); //앞 방향 벡터 계산
	AddMovementInput(Direction, Value); //이동 입력 추가
}
void AQPCharacter::MoveRight(float Value)
{
	MoveInputVector.Y = Value; // 항상 좌 우 입력값 갱신 

	if (!Controller || Value == 0.f) return; //컨트롤러가 없거나

	if (bIsDeathCameraFreeMode)
	{
		HandleDeathCameraInput(FollowCamera ? FollowCamera->GetRightVector() : FVector::ZeroVector, Value);
		return;
	}

	if (Value != 0.f && CurrentRepairingGenerator)
	{
		InteractReleased(); // 모든 종류의 이동 발생 시 수리 중단
	}

	UpdateMovementSpeed(); // 방향이 바뀌면 속도 재계산

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); //컨트롤러의 Yaw 회전 가져오기
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y); //오른쪽 방향 벡터 계산
	AddMovementInput(Direction, Value); //이동 입력 추가
}
void AQPCharacter::Turn(float Value) { AddControllerYawInput(Value); }  //컨트롤러의 Yaw 입력 추가
void AQPCharacter::LookUp(float Value) { AddControllerPitchInput(Value); } //컨트롤러의 Pitch 입력 추가
void AQPCharacter::StartJump() 
{ 
	if (bIsDeathCameraFreeMode)
	{
		HandleDeathCameraInput(FVector(0.f, 0.f, 1.f), 10.f); // 제자리 Z축 위로 상승 (점프량)
		return;
	}

	if (CurrentRepairingGenerator)
	{
		CurrentRepairingGenerator->SubmitSkillCheck(this);
		return;
	}

	Jump(); 
} //점프 시작
void AQPCharacter::StopJump() { StopJumping(); } //점프 멈춤
void AQPCharacter::ToggleCrouch()
{
	if (bIsDeathCameraFreeMode)
	{
		HandleDeathCameraInput(FVector(0.f, 0.f, -1.f), 10.f); // 제자리 Z축 아래로 하강 (앉기량)
		return;
	}
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (!MoveComponent || !MoveComponent->GetNavAgentPropertiesRef().bCanCrouch) return; //무브먼트 컴포넌트가 없거나 앉기 불가능하면 함수 종료
	if (bIsCrouched)
	{
		UnCrouch(); //일어서기
	}
	else
	{
		Crouch(); //앉기
	}
}
void AQPCharacter::StartSprint() //달리기 시작
{
	if (CurrentRepairingGenerator) InteractReleased(); // 수리 중 달리기 시 중단

	if (StatusComponent && !StatusComponent->CanSprint()) return; // 스테미나 부족/고갈 상태면 달리기 시작 불가

	bWantsToSprint = true; 
	UpdateMovementSpeed();
	ServerStartSprint(); 
	
	// [Sprint Effect] 달리기 카메라 쉐이크 시작
	if (SprintCameraShakeClass && IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStartCameraShake(SprintCameraShakeClass);
		}
	}
}
void AQPCharacter::StopSprint() //달리기 멈춤
{
	bWantsToSprint = false; 
	UpdateMovementSpeed(); 
	ServerStopSprint(); 

	// [Sprint Effect] 달리기 카메라 쉐이크 중지 (immediate = false로 부드럽게 감쇄)
	if (SprintCameraShakeClass && IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStopCameraShake(SprintCameraShakeClass, false);
		}
	}
}

void AQPCharacter::ServerStartSprint_Implementation() //서버에서 달리기 시작 처리
{
	bWantsToSprint = true;
	UpdateMovementSpeed();
}

void AQPCharacter::ServerStopSprint_Implementation() //서버에서 달리기 멈춤 처리
{
	bWantsToSprint = false;
	UpdateMovementSpeed();
}

void AQPCharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed(); // SimProxy도 Sprint 상태 변경 시 속도 업데이트
}
void AQPCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) //앉기 시작 시 호출
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnStartCrouch 호출
	UpdateMovementSpeed(); 
}
void AQPCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) //일어서기 시작 시 호출
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnEndCrouch 호출
	UpdateMovementSpeed(); 
}
void AQPCharacter::UpdateMovementSpeed() 
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); 
	if (!MoveComponent) return; 

	// [Add] 무기 종류별 이동 속도 배율 결정
	float WeaponSpeedMultiplier = 1.0f;
	switch (Weapontype)
	{
	case EQPWeaponType::EWT_Handgun:
		WeaponSpeedMultiplier = 0.9f;  // 권총: 10% 감소
		break;
	case EQPWeaponType::EWT_Rifle:
		WeaponSpeedMultiplier = 0.75f; // 라이플: 25% 감소
		break;
	case EQPWeaponType::EWT_Shotgun:
		WeaponSpeedMultiplier = 0.7f;  // 샷건: 30% 감소
		break;
	default:
		WeaponSpeedMultiplier = 1.0f;  // 근접/맨손: 감소 없음
		break;
	}

	/**
	 * 달리기 가능 조건 판단 로직:
	 * 1. 사용자가 달리기 키를 누르고 있음 (bWantsToSprint)
	 * 2. 스태미나가 충분함 (CanSprint)
	 * 3. 조준 중이 아님 (IsAiming)
	 * 4. 공격 애니메이션이 재생 중이 아님 (IsAttacking)
	 * 5. 캐릭터 전방 방향으로 이동 중임 (MoveInputVector.X >= 0)
	 */
	bool bCanSprintState = bWantsToSprint && StatusComponent && StatusComponent->CanSprint() && !IsAiming();
	if (CombatComponent && CombatComponent->IsAttacking()) bCanSprintState = false;
	
	// 뒤로 걷거나 대각선 뒤로 이동할 때는 달리기 속도를 적용하지 않음 (현실성 부여)
	if (MoveInputVector.X < 0.f)
	{
		bCanSprintState = false;
	}

	if (bCanSprintState) 
	{
		float TargetSprintSpeed = SprintSpeed;
		float TargetCrouchSprintSpeed = CrouchSprintSpeed;

		/** 스태미나가 일정 수치 이하(40%)로 떨어지면 지친 상태를 표현하기 위해 속도를 점진적으로 줄임 */
		if (StatusComponent && StatusComponent->GetCurrentStamina() <= 40.f)
		{
			float StaminaRatio = FMath::Clamp(StatusComponent->GetCurrentStamina() / 40.f, 0.f, 1.f);
			TargetSprintSpeed = FMath::Lerp(WalkSpeed, SprintSpeed, StaminaRatio);
			TargetCrouchSprintSpeed = FMath::Lerp(CrouchSpeed, CrouchSprintSpeed, StaminaRatio);
		}

		if (bIsCrouched) 
		{
			MoveComponent->MaxWalkSpeedCrouched = TargetCrouchSprintSpeed * WeaponSpeedMultiplier; 
		}
		else
		{
			MoveComponent->MaxWalkSpeed = TargetSprintSpeed * WeaponSpeedMultiplier; 
		}
	}
	else if (bIsCrouched) 
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed * WeaponSpeedMultiplier; 
	}
	else 
	{
		MoveComponent->MaxWalkSpeed = WalkSpeed * WeaponSpeedMultiplier; 
	}
}

//전투 (Combat) 관련 함수들
void AQPCharacter::EquipPressed()
{
	bEquipKeyDown = true;
	bEquipHoldConsumed = false;

	GetWorldTimerManager().SetTimer(EquipHoldTimerHandle, this, &AQPCharacter::OnEquipHoldTriggered, EquipHoldThreshhold, false);
}

void AQPCharacter::EquipReleased()
{
	if (!bEquipKeyDown) return;
	bEquipKeyDown = false;

	GetWorldTimerManager().ClearTimer(EquipHoldTimerHandle);

	if (!bEquipHoldConsumed)
	{
		TryEquipWeapon();
	}
}

void AQPCharacter::OnEquipHoldTriggered()
{
	if (!bEquipKeyDown) return;
	bEquipHoldConsumed = true;

	TryStorePickupToInventory();
}

void AQPCharacter::TryEquipWeapon() {
	if (!CombatComponent) return; 

	if (OverlappingWeapon)
	{
		if (HasAuthority())
		{
			CombatComponent->EquipWeapon(OverlappingWeapon, true); //겹쳐진 무기 장착 시도
		}
		else
		{
			ServerEquipOverlappingWeapon(OverlappingWeapon); // 서버에 장착 요청
		}

		OverlappingWeapon = nullptr; //겹쳐진 무기 초기화
		RefreshPickupCandidate(nullptr); //픽업 후보 갱신
		return; //함수 종료
	}

	if (OverlappingWorldItem && InventoryComponent) //월드 아이템이 겹쳐져 있고 인벤토리 컴포넌트가 있으면
	{
		UItemDataAsset* ItemData = OverlappingWorldItem->ItemData; //겹쳐진 월드 아이템의 아이템 데이터 가져오기
		const int32 Quantity = OverlappingWorldItem->Quantity; //겹쳐진 월드 아이템의 수량 가져오기

		if (ItemData && Quantity > 0) //아이템 데이터가 유효하고 수량이 0보다 크면
		{
			// [Server Authoritative] 클라이언트에서 직접 추가하던 로직 제거 (복제 버그 원인)
			AWorldItemActor* Picked = OverlappingWorldItem; 
			OverlappingWorldItem = nullptr; 
			UpdatePickupWidgetTarget(); 

			if (Picked)
			{
				if (HasAuthority())
				{
					// 서버인 경우 즉시 추가 시도하고, 성공했을 때만 액터 파괴
					if (InventoryComponent->AddItem(ItemData, Quantity, Picked->AssignedSlotIndex, Picked->AssignedCodeNumber))
					{
						Picked->Destroy();
					}
					else
					{
					}
				}
				else
				{
					// 클라이언트인 경우 서버에 요청만 함
					ServerDestroyPickupActor(Picked, Picked->AssignedSlotIndex, Picked->AssignedCodeNumber);
				}
			}

			RefreshPickupCandidate(Picked); 
		}
	}
}

void AQPCharacter::EquipSlot1() { EquipWeaponByType(EQPWeaponType::EWT_Rifle); }
void AQPCharacter::EquipSlot2() { EquipWeaponByType(EQPWeaponType::EWT_Shotgun); }
void AQPCharacter::EquipSlot3() { EquipWeaponByType(EQPWeaponType::EWT_Handgun); }
void AQPCharacter::EquipSlot4() { EquipWeaponByType(EQPWeaponType::EWT_Melee); }
void AQPCharacter::EquipSlot5() { EquipWeaponByType(EQPWeaponType::EWT_None); }
void AQPCharacter::HolsterWeapon() { EquipWeaponByType(EQPWeaponType::EWT_None); }

void AQPCharacter::EquipWeaponByType(EQPWeaponType TargetType)
{
	if (!InventoryComponent || !CombatComponent) return;

	// 1. 이미 동일한 분류의 무기를 들고 있는지 체크
	AWeaponBase* CurWeapon = CombatComponent->GetEquippedWeapon();
	if (CurWeapon && CurWeapon->GetWeaponType() == TargetType) return;

	// [Holster] 5번 키 또는 None 요청 시 무기를 인벤토리로 되돌림
	if (TargetType == EQPWeaponType::EWT_None)
	{
		if (CurWeapon)
		{
			UItemDataAsset* CurWeaponData = CurWeapon->GetWeaponItemData();
			bool bStored = false;
			int32 AmmoToSave = -1;
			if (AGunWeapon* Gun = Cast<AGunWeapon>(CurWeapon))
			{
				AmmoToSave = Gun->GetCurrentAmmo();
			}
			if (CurWeaponData)
			{
				if (!HasAuthority())
				{
					bStored = InventoryComponent->AddItem(CurWeaponData, 1, -1, -1, AmmoToSave);
				}
				else
				{
					bStored = true; // 서버(호스트)는 실제 스왑 함수에서 처리함
				}
			}

			if (HasAuthority()) ServerSwapWeapon_Implementation(nullptr, !bStored, FIntPoint(-1, -1), -1);
			else ServerSwapWeapon(nullptr, !bStored, FIntPoint(-1, -1), -1);
		}
		return;
	}

	// 2. 인벤토리에서 목표 타입의 무기 찾기
	FIntPoint TargetCell = FIntPoint(-1, -1);
	bool bFound = false;

	for (const FInventorySlot& Slot : InventoryComponent->Slots)
	{
		if (Slot.Item.ItemData && Slot.Item.ItemData->ItemType == EItemType::EIT_Weapon && Slot.Item.ItemData->WeaponClass)
		{
			bool bMatch = false;
			if (Slot.Item.ItemData->TargetWeaponType == TargetType)
			{
				bMatch = true;
			}
			else if (AWeaponBase* CDO = Slot.Item.ItemData->WeaponClass->GetDefaultObject<AWeaponBase>())
			{
				if (CDO->GetWeaponType() == TargetType)
				{
					bMatch = true;
				}
			}

			if (bMatch)
			{
				TargetCell = Slot.Position;
				bFound = true;
				break;
			}
		}
	}

	if (!bFound)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("[경고] 인벤토리에서 해당 무기 타입(%d)을 찾지 못했습니다! Data Asset의 TargetWeaponType을 확인하세요."), (int32)TargetType));
		return;
	}

	FInventorySlot TargetSlot;
	if (!InventoryComponent->FindSlotContaining(TargetCell, TargetSlot)) return;
	TSubclassOf<AWeaponBase> TargetWeaponClass = TargetSlot.Item.ItemData->WeaponClass;

	// 3. 무기 교체 실행 (서버 RPC를 통해 인벤토리 동기화 포함)
	if (CurWeapon)
	{
		UItemDataAsset* CurWeaponData = CurWeapon->GetWeaponItemData();
		
		int32 AmmoToSave = -1;
		if (AGunWeapon* Gun = Cast<AGunWeapon>(CurWeapon))
		{
			AmmoToSave = Gun->GetCurrentAmmo();
		}

		// [SERVER-SYNC] 인벤토리 제거는 이제 ServerSwapWeapon_Implementation 내부에서 수행합니다.
		bool bStored = true;
		if (CurWeaponData)
		{
			if (!HasAuthority())
			{
				bStored = InventoryComponent->AddItem(CurWeaponData, 1, -1, -1, AmmoToSave);
			}
		}

		if (HasAuthority()) ServerSwapWeapon_Implementation(TargetWeaponClass, !bStored, TargetCell, TargetSlot.Item.CurrentAmmo);
		else ServerSwapWeapon(TargetWeaponClass, !bStored, TargetCell, TargetSlot.Item.CurrentAmmo);
	}
	else
	{
		// 맨손 상태에서 새 무기 꺼내기
		if (HasAuthority()) ServerSwapWeapon_Implementation(TargetWeaponClass, false, TargetCell, TargetSlot.Item.CurrentAmmo);
		else ServerSwapWeapon(TargetWeaponClass, false, TargetCell, TargetSlot.Item.CurrentAmmo);
	}
}

void AQPCharacter::ServerSwapWeapon_Implementation(TSubclassOf<AWeaponBase> NewWeaponClass, bool bDropCurrent, FIntPoint TargetCell, int32 AmmoToLoad)
{
	if (!CombatComponent || !InventoryComponent) return;

	// [SERVER-SYNC] 지정된 인벤토리 슬롯에서 아이템 제거 (서버 권위)
	if (TargetCell.X != -1)
	{
		InventoryComponent->RemoveItemAt(TargetCell);
	}

	// 기존 장착 무기 제거
	if (AWeaponBase* CurWeapon = CombatComponent->GetEquippedWeapon())
	{
		int32 AmmoToSave = -1;
		if (AGunWeapon* Gun = Cast<AGunWeapon>(CurWeapon))
		{
			AmmoToSave = Gun->GetCurrentAmmo();
		}

		if (bDropCurrent)
		{
			CombatComponent->UnEquipWeapon(true); // 바닥에 버림
		}
		else
		{
			// 서버 인벤토리에도 기존 무기를 추가해야 함
			bool bSuccessfullyStored = false;
			if (CurWeapon->GetWeaponItemData())
			{
				bSuccessfullyStored = InventoryComponent->AddItem(CurWeapon->GetWeaponItemData(), 1, -1, -1, AmmoToSave);
			}
			
			if (bSuccessfullyStored)
			{
				CombatComponent->UnEquipWeapon(false); // 손에서 없애기만 함 (클라이언트 인벤에 들어갔으므로)
				CurWeapon->Destroy(); // 월드에서 완전히 삭제
			}
			else
			{
				CombatComponent->UnEquipWeapon(true); // 공간 부족 시 바닥에 버림
			}
		}
	}

	// 새 무기 스폰 (NewWeaponClass가 있을 때만)
	if (NewWeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(NewWeaponClass, Params);
		if (NewWeapon)
		{
			// 만약 장착하려는 무기가 총기고, 저장된 탄약 데이터가 있다면 적용
			if (AGunWeapon* GunWeapon = Cast<AGunWeapon>(NewWeapon))
			{
				if (AmmoToLoad != -1)
				{
					// AddAmmo 대신 덮어씌워야 하므로 바로 대입하거나 Clear 후 Add
					// GunWeapon의 CurrentAmmo를 조작하기 위해 함수를 추가하거나 리셋 후 계산
					// 하지만 AddAmmo는 Max 제한을 받으므로 괜찮지만, 기존에 가득 차 있을 수도 있음.
					// GunWeapon.cpp에 SetCurrentAmmo가 없으니 AddAmmo 음수 양수를 이용해 맞춤
					int32 Diff = AmmoToLoad - GunWeapon->GetCurrentAmmo();
					GunWeapon->AddAmmo(Diff);
				}
			}
			CombatComponent->EquipWeapon(NewWeapon, false);
		}
	}
	else
	{
		// 수납 처리 시 애니메이션 상태 업데이트 등을 위해 브로드캐스트
		if (AQPPlayerController* PC = Cast<AQPPlayerController>(GetController()))
		{
			// 필요한 경우 UI/상태 업데이트
		}
	}
}

//E 홀드(길게): 무기면 인벤 저장(장착 안함), 아이템이면 인벤 추가
void AQPCharacter::TryStorePickupToInventory()
{
	if (!InventoryComponent) return;

	if (OverlappingWeapon)
	{
		UItemDataAsset* WeaponItemData = OverlappingWeapon->GetWeaponItemData(); //겹쳐진 무기의 아이템 데이터 가져오기
		if (!WeaponItemData) return; //아이템 데이터가 없으면 함수 종료

		const bool bAdded = InventoryComponent->AddItem(WeaponItemData, 1); //인벤토리에 무기 아이템 추가 시도
		if (bAdded)
		{
			AWeaponBase* PickedWeapon = OverlappingWeapon; //픽업한 무기 저장

			OverlappingWeapon = nullptr; //겹쳐진 무기 초기화
			UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트

			if (PickedWeapon)
			{
				if (HasAuthority()) PickedWeapon->Destroy(); //무기 액터 파괴
				else ServerDestroyPickupActor(PickedWeapon); //서버에 파괴 요청
			}

			RefreshPickupCandidate(PickedWeapon); //픽업 후보 갱신
		}
		return;
	}

	// 월드 아이템: 인벤토리 추가
	if (OverlappingWorldItem)
	{
		UItemDataAsset* ItemData = OverlappingWorldItem->ItemData; //겹쳐진 월드 아이템의 아이템 데이터 가져오기
		const int32 Quantity = OverlappingWorldItem->Quantity; //겹쳐진 월드 아이템의 수량 가져오기

		if (ItemData && Quantity > 0)
		{
			const bool bAdded = InventoryComponent->AddItem(ItemData, Quantity); //아이템 인벤토리에 추가 시도
			if (bAdded)
			{
				AWorldItemActor* Picked = OverlappingWorldItem; //픽업한 월드 아이템 저장

				OverlappingWorldItem = nullptr; //겹쳐진 월드 아이템 초기화
				UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트

				if (Picked)
				{
					if (HasAuthority()) Picked->Destroy(); //월드 아이템 액터 파괴
					else ServerDestroyPickupActor(Picked); //서버에 파괴 요청
				}

				RefreshPickupCandidate(Picked); //픽업 후보 갱신
			}
		}
	}
}
void AQPCharacter::AttackPressed()
{
	if (CurrentRepairingGenerator) InteractReleased(); // 수리 중 공격 시 중단

	if (bIsDeathCameraFreeMode) return;
	if (CombatComponent) CombatComponent->StartAttack(); //공격 시작
}
void AQPCharacter::AttackReleased()
{
	if (CombatComponent) CombatComponent->StopAttack(); //공격 멈춤
}

void AQPCharacter::DropPressed()
{
	bDropKeyDown = true;
	bDropHoldConsumed = false;

	GetWorldTimerManager().SetTimer(DropHoldTimerHandle, this, &AQPCharacter::OnDropHoldTriggered, DropHoldThreshhold, false);
}

void AQPCharacter::DropReleased()
{
	if (!bDropKeyDown) return;
	bDropKeyDown = false;

	GetWorldTimerManager().ClearTimer(DropHoldTimerHandle);

	if (!bDropHoldConsumed)
	{
		// 짧게 누를 때 처리 (필요시)
	}
}

void AQPCharacter::OnDropHoldTriggered()
{
	if (!bDropKeyDown) return;
	bDropHoldConsumed = true;

	TryDropEquipped();
}

void AQPCharacter::TryDropEquipped()
{
	if (!HasAuthority())
	{
		ServerTryDropEquipped();
		return;
	}

	if (!CombatComponent || !CombatComponent->HasWeapon()) return;

	AWeaponBase* WeaponToDrop = CombatComponent->GetEquippedWeapon();
	if (!WeaponToDrop) return;

	CombatComponent->UnEquipWeapon(true); // true로 변경하여 Multicast로 처리되도록 함

	WeaponToDrop->SetOwner(nullptr);

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(WeaponToDrop->GetRootComponent()))
	{
		// 앞으로 살짝, 위로 아주 살짝만 던지도록 힘을 크게 줄임
		RootPrim->AddImpulse(GetActorForwardVector() * 50.f + GetActorUpVector() * 20.f, NAME_None, true);
	}
}

//조준 버튼을 눌렀을 때 호출
void AQPCharacter::AimButtonPressed()
{
	if (CurrentRepairingGenerator) InteractReleased(); // 수리 중 조준 시 중단

	if (!CombatComponent) return; 

	CombatComponent->SetAiming(true); 
	UpdateMovementSpeed(); 
	
	if (CameraBoom) CameraBoom->bEnableCameraLag = false;
}

//조준 버튼에서 손을 뗐을 때 호출
void AQPCharacter::AimButtonReleased()
{
	if (!CombatComponent) return; 

	CombatComponent->SetAiming(false); 
	UpdateMovementSpeed(); 

	// 조준 해제 시 카메라 랙 다시 활성화
	if (CameraBoom) CameraBoom->bEnableCameraLag = true;
}

//현재 조준 중인지 여부를 외부에서 확인
bool AQPCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->IsAiming(); //전투 컴포넌트가 유효하고 조준 중인지 반환
}

// 네트워크 복제 설정
void AQPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQPCharacter, OverlappingWeapon);
	DOREPLIFETIME(AQPCharacter, OverlappingWorldItem);
	DOREPLIFETIME(AQPCharacter, bWantsToSprint);
	DOREPLIFETIME(AQPCharacter, bIsTurningInPlace);
	DOREPLIFETIME(AQPCharacter, AO_Yaw);
	DOREPLIFETIME(AQPCharacter, NetAimYaw);
	DOREPLIFETIME(AQPCharacter, CollectedPassword);
}

bool AQPCharacter::IsDead() const
{
	return StatusComponent ? StatusComponent->IsDead() : false;
}

bool AQPCharacter::IsSprinting() const
{
	if (StatusComponent && !StatusComponent->CanSprint()) return false; // 고갈 상태면 뛸 수 없음

	// 달리기 조건: 달리기 버튼이 눌려 있고, 조준 중이 아니며, **공격 중이 아니며**, 앞으로 이동 입력이 있는 경우
	bool bIsAttacking = false;
	if (CombatComponent)
	{
		bIsAttacking = CombatComponent->IsAttacking();
	}

	if (!bWantsToSprint || IsAiming() || bIsAttacking) return false;

	// 이동 속도 체크: 충분히 빠르게 움직이고 있는지 확인 (걷기 상태에서 약간의 오차 허용)
	if (GetVelocity().SizeSquared2D() < 10.f) return false; // (10.f는 100cm/s의 제곱으로, 거의 정지 상태를 의미)

	// 달리기 방향 체크: 달리는 방향(Velocity)과 캐릭터가 바라보는 방향(Forward)이 대략 90도 내외인지 확인
	// 즉, 뒤나 옆으로 걷고 있는 게 아니라 '앞'으로 뛰고 있을 때만 Sprint로 인정
	float Dot = FVector::DotProduct(GetVelocity().GetSafeNormal2D(), GetActorForwardVector()); 
	return Dot > 0.1f; 
}

void AQPCharacter::AimOffset(float DeltaTime)
{
	if (IsDead()) return; // 사망 상태에서는 에임 오프셋 및 회전 로직 무시 (캐릭터가 카메라를 따라 도는 현상 방지)

	FRotator AimRotation = FRotator::ZeroRotator; // AimRotation 선언을 함수 시작 부분으로 이동


	// 1. 기본 회전값 획득 ( Pitch: Up(-), Down(+) )
	if (Controller)
	{
		AimRotation = Controller->GetControlRotation();
	}
	else
	{
		AimRotation = GetBaseAimRotation();
	}
	
	// [Network] NetAimYaw 업데이트 (Server/Local)
	if (HasAuthority() || IsLocallyControlled())
	{
		NetAimYaw = AimRotation.Yaw;
	}

	AimRotation.Pitch = FRotator::NormalizeAxis(AimRotation.Pitch); // Pitch 정규화 (0 ~ 360 -> -180 ~ 180)

	bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);

	// Simulated Proxy도 AO 계산을 직접 수행 
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		float TargetPitch = FRotator::NormalizeAxis(AimRotation.Pitch); // SimProxy도 Pitch 정규화
		TargetPitch = bIsHoldingGun ? FMath::Clamp(TargetPitch, -30.f, 40.f) : FMath::Clamp(TargetPitch, -90.f, 90.f);
		AO_Pitch = FMath::FInterpTo(AO_Pitch, TargetPitch, DeltaTime, 20.f);  
		AimRotation.Yaw = NetAimYaw;
	}
	else // Local/Server는 기존 로직 유지 (즉시 반영)
	{
		float Pitch = FRotator::NormalizeAxis(AimRotation.Pitch);
		AO_Pitch = bIsHoldingGun ? FMath::Clamp(Pitch, -30.f, 40.f) : FMath::Clamp(Pitch, -90.f, 90.f);
	}

	// 2. HitTarget 기반 보정 (Yaw Only)
	if (CombatComponent && !CombatComponent->HitTarget.IsZero())
	{
		FVector Start = GetActorLocation();

		// 근접 거리 체크 (2m 이상 거리에서만 LookAt 적용)
		if (FVector::Dist(Start, CombatComponent->HitTarget) > 200.f)
		{
			if (IsAiming()) // 조준 중일 때만 LookAt 적용 (비조준 시에는 기존 회전 유지)
			{
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, CombatComponent->HitTarget); // HitTarget을 향하는 회전 계산
				AimRotation.Yaw = LookAtRotation.Yaw; // Yaw만 적용하여 AimRotation 보정
			}
		}
	}

	// 3. Yaw 계산
	const float AimYaw = AimRotation.Yaw;
	const float ActorYaw = GetActorRotation().Yaw;

	const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(
		FRotator(0.f, AimYaw, 0.f),
		FRotator(0.f, ActorYaw, 0.f)
	).Yaw;
	// Simulated Proxy는 부드러운 보간 적용, Local/Server는 즉시 반영 (회전 애니메이션이 어색하게 보이는 것을 방지)
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		float TargetYaw = FMath::Clamp(DeltaYaw, -90.f, 90.f);
		AO_Yaw = FMath::FInterpTo(AO_Yaw, TargetYaw, DeltaTime, 5.f); // 10.f -> 5.f (Micro-Jitter Fix)
	}
	else
	{
		AO_Yaw = FMath::Clamp(DeltaYaw, -90.f, 90.f);
	}

	// 이동 중인지 확인
	const float Speed = GetVelocity().Size2D();
	bool bIsAttacking = false;
	if (CombatComponent) bIsAttacking = CombatComponent->IsAttacking();

	const float DeltaYawAbs = FMath::Abs(DeltaYaw); 

	{
		if (Speed > 1.f) // 이동 중이라면 (1.f는 오차 범위)
		{
			bIsTurningInPlace = false;

			const FRotator TargetRotation = FRotator(0.f, AimYaw, 0.f); // 이동 중에는 항상 AimYaw를 향하도록 회전 (달리기 중에도 적용)
			

			if (HasAuthority() || IsLocallyControlled())
			{
				if (!bUseControllerRotationYaw) // Controller Rotation이 비활성화된 경우에만 회전 로직 적용 (달리기 중에는 Controller Rotation이 활성화되어 있으므로 회전 로직 적용 제외)
				{
					if (IsAiming()) // 조준 중일 때만 즉시 회전 (공격 중 강제 회전 제거)
					{
						SetActorRotation(TargetRotation);
					}
					else // 이동 중이지만 조준하지 않을 때는 부드럽게 회전
					{
						// 몽타주 재생 여부 확인
						bool bIsMontagePlaying = false;
						if (GetMesh() && GetMesh()->GetAnimInstance())
						{
							if (CombatComponent && CombatComponent->GetEquippedWeapon())
							{
								UAnimMontage* FireMontage = CombatComponent->GetEquippedWeapon()->GetFireMontage();
								if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(FireMontage))
								{
									bIsMontagePlaying = true;
								}
							}
						}

						float InterpSpeed = 15.f;
						if (bIsAttacking || bIsMontagePlaying) InterpSpeed = 50.f; // [Fix] 공격 중에는 빠르게 회전하여 상체 비틀림 방지

						const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, InterpSpeed); // 이동 중이지만 조준하지 않을 때는 부드럽게 회전 (공격 중 강제 회전 제거)
						SetActorRotation(NewRotation);

						// 회전 후의 Yaw로 AO_Yaw 계산 
						const float NewActorYaw = NewRotation.Yaw;
						const float NewDeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(FRotator(0.f, AimYaw, 0.f), FRotator(0.f, NewActorYaw, 0.f)).Yaw;
						AO_Yaw = FMath::Clamp(NewDeltaYaw, -90.f, 90.f);
					}
				}
			}
		}
		else // 거의 정지 상태라면 제자리 회전 로직 적용
		{
			
			float TurnThreshold = 60.f; 
			if (bIsAttacking) TurnThreshold = 0.f; // [Fix] 공격 중에는 즉시 회전하여 상체 비틀림 방지

			if (DeltaYawAbs > TurnThreshold) // 60도 이상으로 벌어지면 제자리 회전 시작
			{
				bIsTurningInPlace = true;
			}
			else if (DeltaYawAbs < 5.f) // 5도 미만으로 줄어들면 제자리 회전 종료 (부드러운 종료를 위해 완화)
			{
				bIsTurningInPlace = false;
			}

			if (bIsTurningInPlace) // 제자리 회전 로직 적용
			{
				const FRotator TargetRotation = FRotator(0.f, AimYaw, 0.f); 

				float InterpSpeed = 20.f; // 기본 회전 속도
				if (DeltaYawAbs <= 60.f) // 60도 이하에서는 회전 속도를 점점 빠르게 (잔여 회전이 적을수록 더 빠르게)
				{
					InterpSpeed = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 60.f), FVector2D(10.f, 20.f), DeltaYawAbs); // 0도에 가까울수록 20.f에 가깝게, 60도에 가까울수록 10.f에 가깝게 (잔여 회전이 적을수록 더 빠르게)
				}

				if (HasAuthority() || IsLocallyControlled())
				{
					if (DeltaYawAbs < 2.f) // 2도 미만으로 줄어들면 회전을 강제로 맞춰서 제자리 회전 종료 (잔여 회전이 거의 없을 때는 부드러운 종료를 위해 강제 맞춤)
					{
						SetActorRotation(TargetRotation);
						bIsTurningInPlace = false; // 강제 종료
					}
					else // 잔여 회전이 아직 있을 때는 부드럽게 회전
					{
						const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, InterpSpeed);
						SetActorRotation(NewRotation);
					}
				}
			}
		}
	}

	// 제자리 회전 상태 업데이트 완료
}

// 무기 발사 몽타주 재생 함수
void AQPCharacter::PlayFireMontage(bool bAming)
{
	if (!CombatComponent) return; //전투 컴포넌트가 없으면 함수 종료
	
	AWeaponBase* EquippedWeapon = CombatComponent->GetEquippedWeapon(); //장착된 무기 가져오기
	if (!EquippedWeapon) return; //장착된 무기가 없으면 함수 종료

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //애니메이션 인스턴스 가져오기
	UAnimMontage* MontageToPlay = EquippedWeapon->GetFireMontage();
	
	FName SectionName; //재생할 몽타주 섹션 이름 결정

	switch (Weapontype) 
	{
	case EQPWeaponType::EWT_Rifle:
		SectionName = bAming ? FName("RifleAim") : FName("RifleHip");
		break;
	case EQPWeaponType::EWT_Shotgun:
		SectionName = bAming ? FName("ShotgunAim") : FName("ShotgunHip");
		break;
	case EQPWeaponType::EWT_Handgun:
		SectionName = bAming ? FName("RifleAim") : FName("RifleHip");
		break;
	case EQPWeaponType::EWT_Melee:
		if (MeleeAttackIndex == 0) // 첫 번째 공격이면 Attack1 섹션 재생
		{
			SectionName = FName("Attack1");
			MeleeAttackIndex = 1;
		}
		else // 두 번째 공격이면 Attack2 섹션 재생
		{
			SectionName = FName("Attack2");
			MeleeAttackIndex = 0;
		}
		break;
	default:
		SectionName = NAME_None; // 기본 섹션 이름 (무기 타입이 정의되지 않은 경우)
		break;
	}

	if (AnimInstance && MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		if (SectionName != NAME_None)
		{
			AnimInstance->Montage_JumpToSection(SectionName, MontageToPlay);
		}
	}
}

void AQPCharacter::ReloadButtonPressed() //재장전 버튼이 눌렸을 때 호출
{
	if (CombatComponent)
	{
		CombatComponent->Reload();
	}
}

void AQPCharacter::PlayReloadMontage() //재장전 몽타주 재생 함수
{
	if (!CombatComponent || !ReloadMontage) return; //전투 컴포넌트나 재장전 몽타주가 없으면 함수 종료

	// [Fix] 재장전 사운드 노티파이 재생 직전 메모리에 로드된 SC_SFX 사운드 클래스 볼륨 강제 적용
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
		{
			AudioSubsystem->EnsureSoundClassVolumesApplied();
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage) 
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Weapontype) 
		{
		case EQPWeaponType::EWT_Rifle:
			SectionName = FName("Rifle");
			break;
		case EQPWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EQPWeaponType::EWT_Handgun:
			SectionName = FName("Pistol");
			break;
		}

		if (!SectionName.IsNone()) //섹션 이름이 유효하면 해당 섹션으로 점프
		{
			AnimInstance->Montage_JumpToSection(SectionName, ReloadMontage); 
		}
	}
}

void AQPCharacter::HandleAimStateChanged(bool bIsAiming)
{
	UpdateMovementSpeed(); // 조준 상태가 변경되면 속도를 즉시 업데이트 (멀티플레이어 동기화 보장)
}


	// [Health] 체력 서브 시스템 관련 구현 없음 (StatusComponent로 이관)

float AQPCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float DamageApplied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (IsDead()) return DamageApplied; // 이미 죽었다면 무시

	// --- 혈흔 이펙트 스폰 로직 ---
	FVector HitLocation = GetActorLocation(); // 기본적으로 액터 위치
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		HitLocation = PointDamageEvent->HitInfo.ImpactPoint;
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		const FRadialDamageEvent* RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;
		HitLocation = RadialDamageEvent->Origin;
	}

	if (BloodEffectNiagara)
	{
		// bAutoDestroy 매개변수는 기본값이 true이므로 이펙트 재생이 끝나면 자동으로 컴포넌트가 파괴(사라짐)됩니다.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BloodEffectNiagara, HitLocation, GetActorRotation());
	}
	else if (BloodEffectCascade)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BloodEffectCascade, HitLocation, GetActorRotation());
	}
	// -----------------------------

	// --- 피격 사운드 재생 로직 (샷건 다중 페렛 소리 겹침 방지: 0.08초 쿨다운) ---
	if (HitSound)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastHitSoundTime >= 0.08f)
		{
			LastHitSoundTime = CurrentTime;
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
				{
					AudioSubsystem->PlaySoundAtLocation(HitSound, HitLocation);
				}
			}
		}
	}

	if (StatusComponent)
	{
		StatusComponent->ReceiveDamage(DamageApplied);
	}

	return DamageApplied;
}

void AQPCharacter::Die()
{
	if (bAlreadyDead) return;
	bAlreadyDead = true;

	if (HasAuthority() && InventoryComponent)
	{
		// 0. 현재 장착 중인 무기를 먼저 떨어뜨림
		if (CombatComponent && CombatComponent->HasWeapon())
		{
			CombatComponent->UnEquipWeapon(true);
		}

		// 1. 인벤토리의 모든 아이템을 드랍
		TArray<FInventorySlot> TempSlots = InventoryComponent->Slots; // 순회 중 배열 변경 방지 및 안전한 드랍 보장
		for (const FInventorySlot& Slot : TempSlots)
		{
			if (Slot.Item.ItemData)
			{
				// 현재 위치 주변에 뿌리기
				FVector DropLoc = GetActorLocation() + FVector(FMath::RandRange(-80.f, 80.f), FMath::RandRange(-80.f, 80.f), FMath::RandRange(20.f, 60.f));
				
				ServerSpawnWorldItem_Implementation(Slot.Item.ItemData, Slot.Item.Quantity, DropLoc, Slot.Item.AssignedSlotIndex, Slot.Item.AssignedCodeNumber, Slot.Item.CurrentAmmo);
			}
		}
		InventoryComponent->Slots.Empty(); // 클라이언트 동기화를 위해 비움
		InventoryComponent->OnRep_Slots(); // 호스트 플레이어 UI 갱신을 위해 수동 호출
	}

	if (IsLocallyControlled())
	{
		if (AQPPlayerController* PC = Cast<AQPPlayerController>(GetController()))
		{
			PC->ClearAllUI();
		}
	}

	MulticastDie();
	
	if (HasAuthority())
	{
		AQPGameMode* QPGameMode = Cast<AQPGameMode>(GetWorld()->GetAuthGameMode());
		if (QPGameMode)
		{
			QPGameMode->RequestRespawn(this, GetController());
		}
	}
}

void AQPCharacter::HandleDeathCameraInput(FVector MoveDirection, float Value)
{
	if (bIsDeathCameraFreeMode && CameraBoom)
	{
		// 마우스가 바라보는 3D 렌즈 회전 방향(Pitch 포함)을 그대로 가져와 이동 구현 (초당 배율 20.f 사용)
		CameraBoom->AddWorldOffset(MoveDirection * Value * 20.f);
	}
}

void AQPCharacter::UpdatePickupWidgetTarget()
{
	if (!IsLocallyControlled()) return; //로컬 컨트롤러가 아니면 함수 종료
	if (OverlappingWeapon && !IsValid(OverlappingWeapon))
	{
		OverlappingWeapon = nullptr; //유효하지 않으면 초기화
	}
	if (OverlappingWorldItem && !IsValid(OverlappingWorldItem))
	{
		OverlappingWorldItem = nullptr; //유효하지 않으면 초기화
	}

	AActor* NewTarget = nullptr; //새로운 타겟 액터 초기화
	if (OverlappingWeapon) NewTarget = OverlappingWeapon; //겹쳐진 무기가 있으면 무기 설정
	else if (OverlappingWorldItem) NewTarget = OverlappingWorldItem; //겹쳐진 월드 아이템이 있으면 월드 아이템 설정

	if (AQPPlayerController* PlayerController = Cast<AQPPlayerController>(GetController())) //플레이어 컨트롤러로 캐스팅 시도
	{
		PlayerController->SetPickupTarget(NewTarget); //픽업 타겟 설정
	}
}

void AQPCharacter::RefreshPickupCandidate(const AActor* ActorToIgnore)
{
	TArray<AActor*> Overlaps; //겹쳐진 액터 배열
	GetOverlappingActors(Overlaps); //겹쳐진 액터들 가져오기

	AWeaponBase* BestWeapon = nullptr; //최적의 무기 초기화
	AWorldItemActor* BestWorldItem = nullptr; //최적의 월드 아이템 초기화

	float BestWeaponDistSq = TNumericLimits<float>::Max(); //최적의 무기 거리 제곱 초기화
	float BestWorldDistSq = TNumericLimits<float>::Max(); //최적의 월드 아이템 거리 제곱 초기화

	const AWeaponBase* Equipped = CombatComponent ? CombatComponent->GetEquippedWeapon() : nullptr; //장착된 무기 가져오기
	const FVector MyLoc = GetActorLocation(); //자신의 위치 가져오기

	for (AActor* actors : Overlaps) //겹쳐진 액터들 순회
	{
		if (!IsValid(actors) || actors == this) continue; //유효하지 않거나 자기 자신이면 건너뜀
		if (ActorToIgnore && actors == ActorToIgnore) continue; //무시할 액터이면 건너뜀

		if (AWeaponBase* weapons = Cast<AWeaponBase>(actors)) //무기 액터로 캐스팅 시도
		{
			if (Equipped && weapons == Equipped) continue; //이미 장착된 무기이면 건너뜀

			const float DistSq = FVector::DistSquared(weapons->GetActorLocation(), MyLoc); //무기와 자신의 거리 제곱 계산
			if (DistSq < BestWeaponDistSq) //최적의 무기 거리 제곱보다 작으면
			{
				BestWeaponDistSq = DistSq; //최적의 무기 거리 제곱 업데이트
				BestWeapon = weapons; //최적의 무기 업데이트
			}
		}
		else if (AWorldItemActor* worlditems = Cast<AWorldItemActor>(actors)) //월드 아이템 액터로 캐스팅 시도
		{
			const float DistSq = FVector::DistSquared(worlditems->GetActorLocation(), MyLoc); //월드 아이템과 자신의 거리 제곱 계산
			if (DistSq < BestWorldDistSq) //최적의 월드 아이템 거리 제곱보다 작으면
			{
				BestWorldDistSq = DistSq; //최적의 월드 아이템 거리 제곱 업데이트
				BestWorldItem = worlditems; //최적의 월드 아이템 업데이트
			}
		}
	}
	OverlappingWeapon = BestWeapon; //겹쳐진 무기 설정
	OverlappingWorldItem = (BestWeapon ? nullptr : BestWorldItem); //겹쳐진 월드 아이템 설정 (무기가 있으면 무시)

	UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트
}

void AQPCharacter::MulticastDie_Implementation()
{
	/** 
	 * [사망 처리 시퀀스] 
	 * 모든 클라이언트에서 공통적으로 실행되는 사망 시각적 연출 로직입니다.
	 */

	if (DeathSound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlaySoundAtLocation(DeathSound, GetActorLocation());
			}
		}
	}

	// 1. 진행 중인 공격 동작 즉시 중단
	if (CombatComponent)
	{
		CombatComponent->StopAttack();
	}

	// 2. 사망 애니메이션(몽타주) 재생 및 상태 고정
	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		// 기존 재생 중인 장전/발사 등의 애니메이션을 0.1초 내외로 블렌딩하며 정지
		GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, nullptr);

		float MontageDuration = GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
		
		if (MontageDuration > 0.f)
		{
			TWeakObjectPtr<AQPCharacter> WeakThis(this);
			FTimerHandle AnimPauseTimerHandle;
			/** 사망 애니메이션이 끝난 후 쓰러진 상태를 유지하기 위해 2초 후 애니메이션을 정지(Pause) 시킴 */
			GetWorld()->GetTimerManager().SetTimer(AnimPauseTimerHandle, FTimerDelegate::CreateLambda([WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->GetMesh())
				{
					WeakThis->GetMesh()->bPauseAnims = true; 
				}
			}), 2.0f, false);
		}
	}

	// 3. 충돌 설정 변경: 사망한 시체는 더 이상 다른 캐릭터나 카메라와 충돌하지 않도록 설정
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		CapsuleComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	}

	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	}

	// 4. 사망 카메라 연출 시작: 카메라를 캐릭터에서 분리하여 관전 시점(Transition)으로 전환 준비
	if (CameraBoom)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		CameraBoom->DetachFromComponent(DetachRules); // 캐릭터 몸에서 카메라 붐을 분할
		CameraBoom->bDoCollisionTest = false; // 시체 등에 카메라가 떨리는 현상 방지

		// 컨트롤러 입력 상속 해제
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritRoll = false;
		CameraBoom->bInheritYaw = false;
		
		// UpdateDeathCamera 함수에서 처리될 보간 애니메이션 트리거 활성화
		bIsDeathCameraTransitioning = true;
		bIsDeathCameraFreeMode = false;
	}

	// 5. 캐릭터 이동 컴포넌트 비활성화 (물리 엔진 적용 중단)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AQPCharacter::ServerEquipOverlappingWeapon_Implementation(AWeaponBase* Weapon)
{
	if (CombatComponent && Weapon)
	{
		CombatComponent->EquipWeapon(Weapon, true);
	}
}

void AQPCharacter::ServerUpdateHealth_Implementation(float HealAmount, FIntPoint TargetCell)
{
	if (StatusComponent)
	{
		StatusComponent->AddHealth(HealAmount);
	}
	if (InventoryComponent)
	{
		InventoryComponent->RemoveItemAt(TargetCell);
	}
}

void AQPCharacter::ServerDestroyPickupActor_Implementation(AActor* PickupActor, int32 SlotIdx, int32 CodeNum, FIntPoint TargetCell)
{
	if (PickupActor)
	{
		// [매우 중요] 클라이언트가 생성 직후 너무 빨리 획득(루팅)하면, 리플리케이트(Sync) 딜레이로 인해
		// 클라이언트 측 SlotIdx가 -1인 상태로 서버 RPC가 호출될 수 있습니다.
		if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(PickupActor))
		{
			if (WorldItem->AssignedSlotIndex >= 1)
			{
				SlotIdx = WorldItem->AssignedSlotIndex;
				CodeNum = WorldItem->AssignedCodeNumber;
			}
		}

		// 1. 비밀번호 정보 수집 및 복제
		if (SlotIdx >= 1)
		{
			int32 Idx = SlotIdx - 1;
			if (CollectedPassword.Num() <= Idx)
			{
				int32 TargetSize = FMath::Max(4, SlotIdx);
				if (AQPEscapeGameState* GS = GetWorld()->GetGameState<AQPEscapeGameState>())
				{
					if (GS->TotalGenerators > 0) TargetSize = FMath::Max(TargetSize, GS->TotalGenerators);
				}
				int32 OldSize = CollectedPassword.Num();
				CollectedPassword.SetNum(TargetSize);
				for (int i = OldSize; i < TargetSize; ++i) CollectedPassword[i] = -1;
			}

			if (CollectedPassword.IsValidIndex(Idx))
			{
				CollectedPassword[Idx] = CodeNum;
				if (GetNetMode() == NM_ListenServer || GetWorld()->IsPlayInEditor())
				{
					OnRep_CollectedPassword();
				}
			}
		}

		// 2. 서버 측 인벤토리에 아이템 추가
		if (InventoryComponent)
		{
			UItemDataAsset* ItemData = nullptr;
			int32 Quantity = 1;

			if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(PickupActor))
			{
				ItemData = WorldItem->ItemData;
				Quantity = WorldItem->Quantity;
			}
			else if (AWeaponBase* Weapon = Cast<AWeaponBase>(PickupActor))
			{
				ItemData = Weapon->GetWeaponItemData();
				Quantity = 1;
			}
			
			bool bAdded = false;
			if (ItemData)
			{
				if (TargetCell.X >= 0 && TargetCell.Y >= 0)
				{
					bAdded = InventoryComponent->AddItemAt(ItemData, Quantity, TargetCell, SlotIdx, CodeNum);
				}
				else
				{
					bAdded = InventoryComponent->AddItem(ItemData, Quantity, SlotIdx, CodeNum);
				}
			}

			// 인벤토리에 성공적으로 추가되었을 때만 액터 제거
			if (bAdded)
			{
				PickupActor->Destroy();
			}
		}
	}
}

void AQPCharacter::ServerMoveInventoryItem_Implementation(FIntPoint FromCell, FIntPoint ToCell)
{
	if (InventoryComponent)
	{
		InventoryComponent->MoveItem(FromCell, ToCell);
	}
}

void AQPCharacter::ServerSpawnAndEquipWeapon_Implementation(TSubclassOf<AWeaponBase> WeaponClass)
{
	if (!WeaponClass || !CombatComponent) return;

	// 이미 무기를 장착 중이라면 먼저 해제
	if (CombatComponent->HasWeapon()) 
	{
		CombatComponent->UnEquipWeapon(true); 
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 서버에서 무기 액터 스폰 (네트워크를 통해 모든 클라이언트에 복제됨)
	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, Params);
	if (NewWeapon)
	{
		// 스폰된 무기를 컴뱃 컴포넌트에 장착
		if (!CombatComponent->EquipWeapon(NewWeapon, false))
		{
			NewWeapon->Destroy(); // 장착 실패 시 액터 파괴
		}
	}
}

void AQPCharacter::ServerUpdateShield_Implementation(float Amount, FIntPoint ItemPos)
{
	if (StatusComponent)
	{
		StatusComponent->AddShield(Amount);
	}

	// [SERVER-SYNC] 서버 측 인벤토리에서도 아이템 제거
	if (InventoryComponent && ItemPos.X != -1)
	{
		InventoryComponent->RemoveItemAt(ItemPos);
	}
}

void AQPCharacter::ServerSpawnWorldItem_Implementation(UItemDataAsset* ItemData, int32 Quantity, FVector Location, int32 SlotIdx, int32 CodeNum, int32 Ammo)
{
	if (!ItemData) return;
	
	// 무기 타입인 경우 무기 액터로 스폰
	if (ItemData->ItemType == EItemType::EIT_Weapon && ItemData->WeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = nullptr;
		SpawnParams.Instigator = nullptr;

		AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(ItemData->WeaponClass, Location, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedWeapon && Ammo >= 0)
		{
			if (AGunWeapon* Gun = Cast<AGunWeapon>(SpawnedWeapon))
			{
				Gun->SetCurrentAmmo(Ammo);
			}
		}
	}
	else
	{
		// 일반 아이템인 경우 WorldItemActor로 스폰하여 데이터 설정
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AWorldItemActor* Dropped = GetWorld()->SpawnActor<AWorldItemActor>(AWorldItemActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
		if (Dropped)
		{
			Dropped->ItemData = ItemData;
			Dropped->Quantity = Quantity;
			// [Metadata Inheritance] 버릴 때도 키카드 메타데이터를 유지하여 생성
			Dropped->AssignedSlotIndex = SlotIdx;
			Dropped->AssignedCodeNumber = CodeNum;
			
			// 데이터를 넣은 즉시 메쉬 업데이트 호출 (서버용)
			Dropped->UpdateItemMesh();
		}
	}
}

void AQPCharacter::ServerSubmitKeypadPassword_Implementation(AQPEscapeDoor* Door, const TArray<int32>& Password)
{
	if (!Door) return;

	// 인벤토리에 키카드가 하나라도 있는지 검사
	bool bHasCard = false;
	if (InventoryComponent)
	{
		for (const FInventorySlot& Slot : InventoryComponent->Slots)
		{
			if (Slot.Item.AssignedSlotIndex >= 1)
			{
				bHasCard = true;
				break;
			}
		}
	}

	if (!bHasCard)
	{
		// 키카드가 없으면 문을 열 수 없다는 피드백 전송
		if (AQPPlayerController* PC = Cast<AQPPlayerController>(GetController()))
		{
			if (AQPCrosshair* HUD = Cast<AQPCrosshair>(PC->GetHUD()))
			{
				HUD->ShowBigMessage(TEXT("탈출 키카드가 필요합니다!"), 3.f);
			}
		}
		return;
	}

	// 키카드가 있다면 문 열림 시도
	Door->SubmitPassword(Password);
}

void AQPCharacter::ServerTransferInventoryItem_Implementation(UInventoryComponent* SourceInv, UInventoryComponent* TargetInv, FIntPoint FromCell, FIntPoint ToCell)
{
	if (!SourceInv || !TargetInv) return;

	FInventorySlot Slot;
	if (SourceInv->FindSlotAt(FromCell, Slot))
	{
		int32 Qty = Slot.Item.Quantity;
		UItemDataAsset* Data = Slot.Item.ItemData;
		int32 SlotIdx = Slot.Item.AssignedSlotIndex;
		int32 Code = Slot.Item.AssignedCodeNumber;
		int32 Ammo = Slot.Item.CurrentAmmo;

		if (TargetInv->AddItemAt(Data, Qty, ToCell, SlotIdx, Code, Ammo))
		{
			SourceInv->RemoveItemAt(FromCell);
		}
	}
}

void AQPCharacter::ServerDropItem_Implementation(FIntPoint Cell, FVector DropLocation)
{
	if (!InventoryComponent) return;

	FInventorySlot Slot;
	if (!InventoryComponent->FindSlotContaining(Cell, Slot)) return;
	if (!Slot.Item.ItemData) return;

	UItemDataAsset* ItemData = Slot.Item.ItemData;
	int32 Quantity = Slot.Item.Quantity;
	int32 SlotIdx = Slot.Item.AssignedSlotIndex;
	int32 CodeNum = Slot.Item.AssignedCodeNumber;
	int32 CurrentAmmo = Slot.Item.CurrentAmmo;

	// 서버에서 아이템 스폰 (인벤토리에 저장되어 있던 잔여 탄약 수치 100% 동기화)
	ServerSpawnWorldItem_Implementation(ItemData, Quantity, DropLocation, SlotIdx, CodeNum, CurrentAmmo);

	// 서버에서 인벤토리 아이템 제거
	InventoryComponent->RemoveItemAt(Slot.Position);
}

void AQPCharacter::ServerTryDropEquipped_Implementation()
{
	TryDropEquipped();
}

void AQPCharacter::InteractPressed()
{

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		// 1순위: 발전기 조작 (수리 시도)
		if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Actor))
		{
			CurrentRepairingGenerator = Generator;
			
			// [NETWORK-FIX] 클라이언트는 서버 신뢰를 위해 캐릭터 RPC를 통함
			if (HasAuthority()) ServerInteract_Implementation(Generator);
			else ServerInteract(Generator);

			return; 
		}
		
		// 2순위: 탈출문 키패드 조작
		if (AQPEscapeDoor* Door = Cast<AQPEscapeDoor>(Actor))
		{
			bool bHasKeycard = false;
			if (InventoryComponent)
			{
				for (const FInventorySlot& Slot : InventoryComponent->Slots)
				{
					if (Slot.Item.ItemData)
					{
						FString ItemNameStr = Slot.Item.ItemData->ItemName.ToString();
						if (ItemNameStr.Contains(TEXT("키카드")) || ItemNameStr.Contains(TEXT("Keycard")) || Slot.Item.AssignedSlotIndex >= 1)
						{
							bHasKeycard = true;
							break;
						}
					}
				}
			}

			if (!bHasKeycard)
			{
				if (GEngine && IsLocallyControlled())
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("키패드를 조작하려면 키카드가 필요합니다!"));
				}
				return;
			}

			if (HasAuthority()) ServerInteract_Implementation(Door);
			else ServerInteract(Door);

			ShowKeypadUI(Door);
			return; 
		}
	}
}

void AQPCharacter::ServerInteract_Implementation(AActor* Target)
{
	if (!Target) return;

	// 발전기 상호작용인 경우
	if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Target))
	{
		Generator->StartRepairing(this);
	}
	// 탈출문 상호작용인 경우 (필요 시 서버 측 처리 추가)
	else if (AQPEscapeDoor* Door = Cast<AQPEscapeDoor>(Target))
	{
		// 서버 측 문 상호작용 로직 (현재는 UI만 띄우면 됨)
	}
}

void AQPCharacter::InteractReleased()
{
	if (CurrentRepairingGenerator)
	{
		if (HasAuthority()) ServerStopInteract_Implementation(CurrentRepairingGenerator);
		else ServerStopInteract(CurrentRepairingGenerator);

		CurrentRepairingGenerator = nullptr;
	}
}

void AQPCharacter::ServerStopInteract_Implementation(AActor* Target)
{
	if (!Target) return;

	if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Target))
	{
		Generator->StopRepairing(this);
	}
}

void AQPCharacter::ShowStarCatchUI_Implementation(float StartRatio, float WidthRatio, float Duration)
{
	if (!StarCatchWidgetInstance && StarCatchWidgetClass)
	{
		StarCatchWidgetInstance = CreateWidget<UQPGeneratorWidget>(GetWorld(), StarCatchWidgetClass);
		if (StarCatchWidgetInstance)
		{
			StarCatchWidgetInstance->AddToViewport();
		}
	}

	if (StarCatchWidgetInstance)
	{
		StarCatchWidgetInstance->ShowSkillCheckPopup(StartRatio, WidthRatio, Duration);
	}
}

void AQPCharacter::HideStarCatchUI_Implementation()
{
	if (StarCatchWidgetInstance)
	{
		StarCatchWidgetInstance->HideSkillCheckPopup();
		StarCatchWidgetInstance->RemoveFromParent();
		StarCatchWidgetInstance = nullptr;
	}
}

void AQPCharacter::ShowKeypadUI(AQPEscapeDoor* Door)
{
	if (!IsLocallyControlled()) return;

	if (!KeypadWidgetInstance && KeypadWidgetClass)
	{
		KeypadWidgetInstance = CreateWidget<UQPKeypadWidget>(GetWorld(), KeypadWidgetClass);
	}

	if (KeypadWidgetInstance)
	{
		KeypadWidgetInstance->SetTargetDoor(Door);
		if (!KeypadWidgetInstance->IsInViewport())
		{
			KeypadWidgetInstance->AddToViewport();
		}

		// UI 진입 시 이동 키가 고장나는 현상 방지
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->StopMovement();
			PC->FlushPressedKeys();
		}
	}
	else
	{
	}
}

void AQPCharacter::HideKeypadUI()
{
	if (!IsLocallyControlled()) return;

	if (KeypadWidgetInstance && KeypadWidgetInstance->IsInViewport())
	{
		KeypadWidgetInstance->RemoveFromParent();
		
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
		}
	}
}

void AQPCharacter::OnRep_CollectedPassword()
{
	// 서버에서 배열이 변경되었을 때 로컬 UI도 즉시 업데이트 되도록 합니다.
	if (InventoryComponent)
	{
		// 인벤토리 변경 이벤트를 수동으로 발생시켜 연동된 위젯(InventoryRootWidget 등)이 갱신되도록 합니다.
		InventoryComponent->OnInventoryChanged.Broadcast();
	}
}

void AQPCharacter::ServerSpawnAndEquipWeapon_Implementation(TSubclassOf<AWeaponBase> WeaponClass)
{
	if (!WeaponClass || !CombatComponent) return;

	// 이미 무기를 장착 중이라면 먼저 해제
	if (CombatComponent->HasWeapon()) 
	{
		CombatComponent->UnEquipWeapon(true); 
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 서버에서 무기 액터 스폰 (네트워크를 통해 모든 클라이언트에 복제됨)
	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, Params);
	if (NewWeapon)
	{
		// 스폰된 무기를 컴뱃 컴포넌트에 장착
		if (!CombatComponent->EquipWeapon(NewWeapon, false))
		{
			NewWeapon->Destroy(); // 장착 실패 시 액터 파괴
		}
	}
}

void AQPCharacter::ServerUpdateShield_Implementation(float Amount, FIntPoint ItemPos)
{
	if (StatusComponent)
	{
		StatusComponent->AddShield(Amount);
	}

	// [SERVER-SYNC] 서버 측 인벤토리에서도 아이템 제거
	if (InventoryComponent && ItemPos.X != -1)
	{
		InventoryComponent->RemoveItemAt(ItemPos);
	}
}

void AQPCharacter::ServerSpawnWorldItem_Implementation(UItemDataAsset* ItemData, int32 Quantity, FVector Location, int32 SlotIdx, int32 CodeNum)
{
	if (!ItemData) return;
	
	// 무기 타입인 경우 무기 액터로 스폰
	if (ItemData->ItemType == EItemType::EIT_Weapon && ItemData->WeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = nullptr;
		SpawnParams.Instigator = nullptr;

		GetWorld()->SpawnActor<AWeaponBase>(ItemData->WeaponClass, Location, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		// 일반 아이템인 경우 WorldItemActor로 스폰하여 데이터 설정
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AWorldItemActor* Dropped = GetWorld()->SpawnActor<AWorldItemActor>(AWorldItemActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
		if (Dropped)
		{
			Dropped->ItemData = ItemData;
			Dropped->Quantity = Quantity;
			// [Metadata Inheritance] 버릴 때도 키카드 메타데이터를 유지하여 생성
			Dropped->AssignedSlotIndex = SlotIdx;
			Dropped->AssignedCodeNumber = CodeNum;
		}
	}
}

void AQPCharacter::ServerTryDropEquipped_Implementation()
{
	TryDropEquipped();
}

void AQPCharacter::InteractPressed()
{

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		// 1순위: 발전기 조작 (수리 시도)
		if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Actor))
		{
			CurrentRepairingGenerator = Generator;
			
			// [NETWORK-FIX] 클라이언트는 서버 신뢰를 위해 캐릭터 RPC를 통함
			if (HasAuthority()) ServerInteract_Implementation(Generator);
			else ServerInteract(Generator);

			return; 
		}
		
		// 2순위: 탈출문 키패드 조작
		if (AQPEscapeDoor* Door = Cast<AQPEscapeDoor>(Actor))
		{
			if (HasAuthority()) ServerInteract_Implementation(Door);
			else ServerInteract(Door);

			ShowKeypadUI(Door);
			return; 
		}
	}
}

void AQPCharacter::ServerInteract_Implementation(AActor* Target)
{
	if (!Target) return;

	// 발전기 상호작용인 경우
	if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Target))
	{
		Generator->StartRepairing(this);
	}
	// 탈출문 상호작용인 경우 (필요 시 서버 측 처리 추가)
	else if (AQPEscapeDoor* Door = Cast<AQPEscapeDoor>(Target))
	{
		// 서버 측 문 상호작용 로직 (현재는 UI만 띄우면 됨)
	}
}

void AQPCharacter::InteractReleased()
{
	if (CurrentRepairingGenerator)
	{
		if (HasAuthority()) ServerStopInteract_Implementation(CurrentRepairingGenerator);
		else ServerStopInteract(CurrentRepairingGenerator);

		CurrentRepairingGenerator = nullptr;
	}
}

void AQPCharacter::ServerStopInteract_Implementation(AActor* Target)
{
	if (!Target) return;

	if (AQPEscapeGenerator* Generator = Cast<AQPEscapeGenerator>(Target))
	{
		Generator->StopRepairing(this);
	}
}

void AQPCharacter::ShowStarCatchUI_Implementation(float StartRatio, float WidthRatio, float Duration)
{
	if (!StarCatchWidgetInstance && StarCatchWidgetClass)
	{
		StarCatchWidgetInstance = CreateWidget<UQPGeneratorWidget>(GetWorld(), StarCatchWidgetClass);
		if (StarCatchWidgetInstance)
		{
			StarCatchWidgetInstance->AddToViewport();
		}
	}

	if (StarCatchWidgetInstance)
	{
		StarCatchWidgetInstance->ShowSkillCheckPopup(StartRatio, WidthRatio, Duration);
	}
}

void AQPCharacter::HideStarCatchUI_Implementation()
{
	if (StarCatchWidgetInstance)
	{
		StarCatchWidgetInstance->HideSkillCheckPopup();
		StarCatchWidgetInstance->RemoveFromParent();
		StarCatchWidgetInstance = nullptr;
	}
}

void AQPCharacter::ShowKeypadUI(AQPEscapeDoor* Door)
{
	if (!IsLocallyControlled()) return;

	if (!KeypadWidgetInstance && KeypadWidgetClass)
	{
		KeypadWidgetInstance = CreateWidget<UQPKeypadWidget>(GetWorld(), KeypadWidgetClass);
	}

	if (KeypadWidgetInstance)
	{
		KeypadWidgetInstance->SetTargetDoor(Door);
		if (!KeypadWidgetInstance->IsInViewport())
		{
			KeypadWidgetInstance->AddToViewport();
		}

		// UI 진입 시 이동 키가 고장나는 현상 방지
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->StopMovement();
			PC->FlushPressedKeys();
		}
	}
	else
	{
	}
}

void AQPCharacter::HideKeypadUI()
{
	if (!IsLocallyControlled()) return;

	if (KeypadWidgetInstance && KeypadWidgetInstance->IsInViewport())
	{
		KeypadWidgetInstance->RemoveFromParent();
		
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
		}
	}
}

void AQPCharacter::OnRep_CollectedPassword()
{
	// 서버에서 배열이 변경되었을 때 로컬 UI도 즉시 업데이트 되도록 합니다.
	if (InventoryComponent)
	{
		// 인벤토리 변경 이벤트를 수동으로 발생시켜 연동된 위젯(InventoryRootWidget 등)이 갱신되도록 합니다.
		InventoryComponent->OnInventoryChanged.Broadcast();
	}
}

