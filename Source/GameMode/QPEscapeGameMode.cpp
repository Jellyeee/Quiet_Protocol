#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeDoor.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeGenerator.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/UserWidget/Crosshair/QPCrosshair.h"

AQPEscapeGameMode::AQPEscapeGameMode()
{
}

void AQPEscapeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 1. 맵에 배치된 출구 캐싱
	// (발전기는 이제 자기 자신의 BeginPlay()에서 직접 게임모드에 스스로를 등록합니다)

	TArray<AActor*> FoundDoors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQPEscapeDoor::StaticClass(), FoundDoors);
	for (AActor* Actor : FoundDoors)
	{
		if (AQPEscapeDoor* Door = Cast<AQPEscapeDoor>(Actor))
		{
			AllDoors.Add(Door);
		}
	}

	// 2. 문 개수에 맞춰 진짜 출구 지정
	SetRealExits();
}

void AQPEscapeGameMode::RegisterGenerator(AQPEscapeGenerator* Generator)
{
	if (!Generator) return;

	if (!AllGenerators.Contains(Generator))
	{
		AllGenerators.Add(Generator);
		
		int32 NewSlotIdx = AllGenerators.Num();
		int32 NewCode = FMath::RandRange(0, 9);
		MasterPassword.Add(NewCode);

		Generator->AssignCardInfo(NewSlotIdx, NewCode);

		// GameState 갱신
		if (AQPEscapeGameState* GS = GetGameState<AQPEscapeGameState>())
		{
			GS->TotalGenerators = AllGenerators.Num();
			if (GS->RemainingGenerators < GS->TotalGenerators)
			{
				GS->RemainingGenerators = AllGenerators.Num();
			}
		}


	}
}



void AQPEscapeGameMode::SetRealExits()
{
	if (AllDoors.Num() == 0) return;

	// 기본적으로 모두 오답 출구로 설정
	for (AQPEscapeDoor* Door : AllDoors)
	{
		Door->SetIsRealExit(false);
	}

	// 4개당 1개씩 진짜 출구 지정 (최소 1개)
	int32 RealExitCount = FMath::Max(1, AllDoors.Num() / 4);
	
	TArray<int32> Indices;
	for (int32 i = 0; i < AllDoors.Num(); ++i) Indices.Add(i);

	// 랜덤하게 인덱스 셔플하여 필요한 수만큼 진짜 출구로 설정
	for (int32 i = 0; i < RealExitCount && Indices.Num() > 0; ++i)
	{
		int32 RandomIdx = FMath::RandRange(0, Indices.Num() - 1);
		int32 DoorIdx = Indices[RandomIdx];
		
		if (AllDoors.IsValidIndex(DoorIdx))
		{
			AllDoors[DoorIdx]->SetIsRealExit(true);

		}
		
		Indices.RemoveAt(RandomIdx);
	}
}



bool AQPEscapeGameMode::VerifyEscapeAttempt(const TArray<int32>& InputPassword, AQPEscapeDoor* TargetDoor)
{
	if (!TargetDoor) return false;

	// 1. 패스워드 일치 여부 확인
	bool bIsPasswordCorrect = true;
	if (InputPassword.Num() != MasterPassword.Num())
	{
		bIsPasswordCorrect = false;
	}
	else
	{
		for (int32 i = 0; i < MasterPassword.Num(); ++i)
		{
			if (InputPassword[i] != MasterPassword[i])
			{
				bIsPasswordCorrect = false;
				break;
			}
		}
	}

	// 2. 결과 처리
	if (bIsPasswordCorrect)
	{
		if (TargetDoor->IsRealExit())
		{
			// 정답 & 진짜 문 -> 정상 탈출
			TargetDoor->OpenDoor();
			return true;
		}
		else
		{
			// 정답 & 가짜 문 -> 가짜 문 개방 및 좀비 웨이브 스폰
			TargetDoor->TriggerTrap();
			return false;
		}
	}
	else
	{
		// 오답 -> 가짜 문은 열리지 않고 강력한 소음으로 주변 좀비 광역 어그로 유발
		TargetDoor->TriggerWrongPasswordAggro();
		return false;
	}
}

void AQPEscapeGameMode::OnPlayerEscaped(AQPCharacter* EscapedPlayer)
{
	if (!EscapedPlayer) return;

	APlayerController* PC = Cast<APlayerController>(EscapedPlayer->GetController());
	if (PC)
	{
		// 1. 플레이어 제어권 회수
		EscapedPlayer->DisableInput(PC);

		// HUD에 탈출 성공 메시지 표시
		if (AQPCrosshair* HUD = Cast<AQPCrosshair>(PC->GetHUD()))
		{
			HUD->ShowBigMessage(TEXT("안전하게 탈출했습니다!"), 5.f);
		}

		// 2. 다른 플레이어를 관전할 수 있도록 관전 모드 부여
		PC->ChangeState(NAME_Spectating);
		PC->ClientGotoState(NAME_Spectating);

		// 3. 탈출한 플레이어의 캐릭터 모델을 맵에서 숨김/무적 처리 (또는 Destroy)
		EscapedPlayer->SetActorHiddenInGame(true);
		EscapedPlayer->SetActorEnableCollision(false);


	}
}
