#include "PJ_Quiet_Protocol/Environment/QPEscapeGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"
#include "PJ_Quiet_Protocol/UI/QPGeneratorWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameMode.h"

AQPEscapeGenerator::AQPEscapeGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractVolume"));
	InteractVolume->SetBoxExtent(FVector(200.f, 200.f, 150.f));
	InteractVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(InteractVolume);

	GeneratorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GeneratorMesh"));
	GeneratorMesh->SetupAttachment(RootComponent);

	ProgressWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("ProgressWidgetComp"));
	ProgressWidgetComp->SetupAttachment(RootComponent);
	ProgressWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 화면에 항상 보이게
	ProgressWidgetComp->SetDrawSize(FVector2D(500.f, 200.f)); // 크기를 충분히 키움 (중요)
	ProgressWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 150.f)); // 발전기 위
	ProgressWidgetComp->SetVisibility(false); // 처음엔 숨김

	CurrentProgress = 0.f;
	bIsCompleted = false;
	bIsBeingRepaired = false;
	AssignedSlotIndex = -1;
	AssignedCodeNumber = -1;
	CurrentRepairer = nullptr;

	bIsSkillCheckActive = false;
	SkillCheckMaxTime = 1.5f;
}

void AQPEscapeGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (AQPEscapeGameMode* GM = Cast<AQPEscapeGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->RegisterGenerator(this);
		}
	}
}

void AQPEscapeGenerator::AssignCardInfo(int32 SlotIdx, int32 CodeNum)
{
	AssignedSlotIndex = SlotIdx;
	AssignedCodeNumber = CodeNum;
}

void AQPEscapeGenerator::AddProgress(float Amount)
{
	if (bIsCompleted) return;

	if (HasAuthority())
	{
		CurrentProgress += Amount;


		if (CurrentProgress >= MaxProgress)
		{
			CurrentProgress = MaxProgress;
			CompleteRepair();
		}
		
		// 호스트(서버)가 로컬에서 수리할 때 즉각 갱신
		OnRep_CurrentProgress();
	}
}

void AQPEscapeGenerator::OnRep_CurrentProgress()
{
	if (UQPGeneratorWidget* GenWidget = Cast<UQPGeneratorWidget>(ProgressWidgetComp->GetUserWidgetObject()))
	{
		GenWidget->UpdateRepairProgress(GetProgressRatio());
	}
}

void AQPEscapeGenerator::OnRep_IsBeingRepaired()
{
	if (bIsCompleted)
	{
		ProgressWidgetComp->SetVisibility(false);
		return;
	}

	ProgressWidgetComp->SetVisibility(bIsBeingRepaired);
}

void AQPEscapeGenerator::OnRep_IsCompleted()
{
	if (bIsCompleted && ProgressWidgetComp)
	{
		ProgressWidgetComp->SetVisibility(false);
	}
}

void AQPEscapeGenerator::StartRepairing(AQPCharacter* Character)
{
	if (bIsCompleted) return;
	
	if (HasAuthority())
	{
		ServerStartRepairing_Implementation(Character);
	}
	else
	{
		ServerStartRepairing(Character);
	}
}

void AQPEscapeGenerator::ServerStartRepairing_Implementation(AQPCharacter* Character)
{
	if (bIsCompleted || CurrentRepairer != nullptr) return;

	CurrentRepairer = Character;
	bIsBeingRepaired = true;
	OnRep_IsBeingRepaired(); // 서버(호스트)에서도 즉시 반영

	SetOwner(Character); // Client RPC 호출을 위해 소유권 임시 할당
	GetWorldTimerManager().SetTimer(RepairTimerHandle, this, &AQPEscapeGenerator::RepairTick, 0.1f, true);
}

void AQPEscapeGenerator::StopRepairing(AQPCharacter* Character)
{
	if (HasAuthority())
	{
		ServerStopRepairing_Implementation(Character);
	}
	else
	{
		ServerStopRepairing(Character);
	}
}

void AQPEscapeGenerator::ServerStopRepairing_Implementation(AQPCharacter* Character)
{
	if (CurrentRepairer == Character)
	{
		if (bIsSkillCheckActive)
		{
			// 수리 중단 시 스킬체크가 진행 중이었다면 무조건 실패 처리하여 UI를 닫음
			FailSkillCheck();
		}

		CurrentRepairer = nullptr;
		bIsBeingRepaired = false;
		OnRep_IsBeingRepaired(); // 서버(호스트)에서도 즉시 반영

		SetOwner(nullptr);
		GetWorldTimerManager().ClearTimer(RepairTimerHandle);
	}
}

void AQPEscapeGenerator::SubmitSkillCheck(AQPCharacter* Character)
{
	if (HasAuthority()) ServerSubmitSkillCheck_Implementation(Character);
	else ServerSubmitSkillCheck(Character);
}

void AQPEscapeGenerator::ServerSubmitSkillCheck_Implementation(AQPCharacter* Character)
{
	if (!bIsSkillCheckActive || CurrentRepairer != Character) return;

	// 바늘 위치 퍼센트 (진행률) 계산
	float NeedleRatio = 1.0f - (SkillCheckTimeLeft / SkillCheckMaxTime);

	if (NeedleRatio >= SkillCheckTargetStart && NeedleRatio <= SkillCheckTargetEnd)
	{
		// 스킬체크 성공 보너스
		CurrentProgress += 3.f; 
	}
	else
	{
		// 빗나감 (실패)
		FailSkillCheck();
	}

	bIsSkillCheckActive = false;
	if (CurrentRepairer)
	{
		CurrentRepairer->HideStarCatchUI();
	}
}

void AQPEscapeGenerator::TriggerSkillCheck()
{
	bIsSkillCheckActive = true;
	SkillCheckMaxTime = FMath::RandRange(1.2f, 2.0f);
	SkillCheckTimeLeft = SkillCheckMaxTime;

	// 타겟 존 (0.1 ~ 0.3 너비로 랜덤)
	float TargetWidth = FMath::RandRange(0.1f, 0.25f);
	// 0.2 ~ 0.8 사이 랜덤 구역에 타겟 생성
	SkillCheckTargetStart = FMath::RandRange(0.2f, 0.8f - TargetWidth);
	SkillCheckTargetEnd = SkillCheckTargetStart + TargetWidth;

	if (CurrentRepairer)
	{
		CurrentRepairer->ShowStarCatchUI(SkillCheckTargetStart, TargetWidth, SkillCheckMaxTime);
	}
}

void AQPEscapeGenerator::FailSkillCheck()
{
	bIsSkillCheckActive = false;
	if (CurrentRepairer)
	{
		CurrentRepairer->HideStarCatchUI();
	}

	// 수리 게이지 10% 삭감
	CurrentProgress = FMath::Max(0.f, CurrentProgress - 10.f);

	// 어그로 핑 및 소음 발생
	MulticastSkillCheckFailed();
}

void AQPEscapeGenerator::RepairTick()
{
	if (!CurrentRepairer || bIsCompleted)
	{
		GetWorldTimerManager().ClearTimer(RepairTimerHandle);
		if (CurrentRepairer) SetOwner(nullptr);
		return;
	}

	if (bIsSkillCheckActive)
	{
		SkillCheckTimeLeft -= 0.1f;
		if (SkillCheckTimeLeft <= 0.f)
		{
			// 시간 만료 실패
			FailSkillCheck();
		}
	}
	else
	{
		// 스킬체크가 없을 때 0.1초당 약 1.5% 확률로 스킬체크 발동
		if (FMath::RandRange(0, 1000) < 15)
		{
			TriggerSkillCheck();
		}
	}

	// 0.1초마다 진행도 증가 (초당 진행도 / 10)
	AddProgress(RepairRatePerSecond * 0.1f);
}

void AQPEscapeGenerator::CompleteRepair()
{
	if (bIsCompleted) return;
	bIsCompleted = true;

	if (bIsSkillCheckActive)
	{
		bIsSkillCheckActive = false;
	}

	if (CurrentRepairer)
	{
		CurrentRepairer->HideStarCatchUI();
	}

	bIsBeingRepaired = false; // 서버 상태 정리
	CurrentRepairer = nullptr; // 서버 상태 정리

	// 타이머 정리
	GetWorldTimerManager().ClearTimer(RepairTimerHandle);

	// 아이템 드롭 로직 호출
	DropRewards();

	// 시각 및 청각 피드백 발송
	MulticastCompleteRepair();

	// 수리 완료 시 위젯 숨김
	if (ProgressWidgetComp)
	{
		ProgressWidgetComp->SetVisibility(false);
	}

	// GameState의 남은 발전기 수 감소
	if (AQPEscapeGameState* GS = GetWorld()->GetGameState<AQPEscapeGameState>())
	{
		GS->RemainingGenerators = FMath::Max(0, GS->RemainingGenerators - 1);
	}
}

void AQPEscapeGenerator::MulticastCompleteRepair_Implementation()
{
	// [Effect] 발전기 불 켜짐, 소리에펙트 등 시각적 처리 (모든 클라이언트)
	if (ProgressWidgetComp)
	{
		ProgressWidgetComp->SetVisibility(false);
	}

	// 수리 완료 시 아무 키도 누르지 않았어도 화면에 미니게임 UI가 남지 않도록 100% 강제 닫기
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (AQPCharacter* Character = Cast<AQPCharacter>(PC->GetPawn()))
		{
			Character->HideStarCatchUI();
		}
	}

	if (GeneratorMesh)
	{
		// 예: GeneratorMesh->SetScalarParameterValueOnMaterials(TEXT("EmissiveStrength"), 10.f);
	}
}

void AQPEscapeGenerator::MulticastSkillCheckFailed_Implementation()
{
	// [Effect] 파편 튀는 파티클, 큰 폭발음 재생 처리 (모든 클라이언트)

}

void AQPEscapeGenerator::DropRewards()
{
	if (!HasAuthority()) return;

	// 발전기 근처 바닥에 생성 (기본 위치: 정면 150 유닛 앞)
	FVector BaseSpawnLocation = GetActorLocation() + (GetActorForwardVector() * 150.f) + FVector(0, 0, 30.f);

	for (int32 i = 0; i < RewardItems.Num(); ++i)
	{
		const FRewardItem& Reward = RewardItems[i];
		if (!Reward.ItemClass || !Reward.ItemData) continue;

		// 아이템들이 겹치지 않게 가로(RightVector) 방향으로 분산 배치
		float Offset = (i * 40.f) - ((RewardItems.Num() - 1) * 20.f);
		FVector FinalSpawnLocation = BaseSpawnLocation + (GetActorRightVector() * Offset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AWorldItemActor* DroppedItem = GetWorld()->SpawnActor<AWorldItemActor>(Reward.ItemClass, FinalSpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (DroppedItem)
		{
			DroppedItem->ItemData = Reward.ItemData;
			// 에러 발생을 막기 위한 안전장치 (임시 변수로 처리하여 멤버 변수가 오염되지 않게 함)
			int32 TempSlotIndex = (AssignedSlotIndex <= 0) ? 1 : AssignedSlotIndex;
			int32 TempCodeNumber = (AssignedCodeNumber < 0) ? 0 : AssignedCodeNumber;

			DroppedItem->AssignedSlotIndex = TempSlotIndex;
			DroppedItem->AssignedCodeNumber = TempCodeNumber;

			// 데이터를 넣은 즉시 메쉬 업데이트 호출 (서버용)
			DroppedItem->UpdateItemMesh();
		}
	}
}

void AQPEscapeGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQPEscapeGenerator, CurrentProgress);
	DOREPLIFETIME(AQPEscapeGenerator, bIsCompleted);
	DOREPLIFETIME(AQPEscapeGenerator, bIsBeingRepaired);
}
