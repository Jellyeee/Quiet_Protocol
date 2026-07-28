#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameMode.h"
#include "PJ_Quiet_Protocol/Audio/QPAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeDoor.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeGenerator.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/UserWidget/Crosshair/QPCrosshair.h"
#include "PJ_Quiet_Protocol/Environment/QPEscapeGeneratorSpawnPoint.h"
#include "GameFramework/PlayerState.h"
#include "Engine/DamageEvents.h"

AQPEscapeGameMode::AQPEscapeGameMode()
{
	bGameFinished = false;
}

void AQPEscapeGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (InGameBGM)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQPAudioSubsystem* AudioSubsystem = GI->GetSubsystem<UQPAudioSubsystem>())
			{
				AudioSubsystem->PlayBGM(InGameBGM);
			}
		}
	}

	// 0. 발전기 스폰 포인트에서 무작위로 발전기 생성
	if (GeneratorClass)
	{
		TArray<AActor*> FoundPoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQPEscapeGeneratorSpawnPoint::StaticClass(), FoundPoints);

		if (FoundPoints.Num() > 0)
		{

			// 배열 랜덤 셔플
			for (int32 i = FoundPoints.Num() - 1; i > 0; i--)
			{
				int32 j = FMath::RandRange(0, i);
				FoundPoints.Swap(i, j);
			}

			// 지정된 개수만큼 스폰
			int32 SpawnCount = FMath::Min(NumGeneratorsToSpawn, FoundPoints.Num());
			for (int32 i = 0; i < SpawnCount; i++)
			{
				AActor* SpawnPoint = FoundPoints[i];
				if (SpawnPoint)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					GetWorld()->SpawnActor<AQPEscapeGenerator>(GeneratorClass, SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation(), SpawnParams);
				}
			}
		}
	}

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
	if (!EscapedPlayer || bGameFinished) return;

	APlayerController* PC = Cast<APlayerController>(EscapedPlayer->GetController());
	
	// [Fix] 서버에서 클라이언트 캐릭터의 GetController()가 순간적으로 null이거나 다른 형태일 수 있으므로 전역 검색 보완
	if (!PC)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* TestPC = It->Get())
			{
				if (TestPC->GetPawn() == EscapedPlayer)
				{
					PC = TestPC;
					break;
				}
			}
		}
	}

	if (PC)
	{
		// 1. 게임 종료 상태 설정 (최초 탈출자가 나오면 즉시 게임 종료)
		bGameFinished = true;

		// 2. 탈출 성공 플레이어 목록 추가
		EscapedPlayers.AddUnique(PC);

		// 3. 탈출한 플레이어 제어권 회수 및 관전 모드 전환
		EscapedPlayer->DisableInput(PC);
		PC->ChangeState(NAME_Spectating);
		PC->ClientGotoState(NAME_Spectating);

		// 4. 탈출한 플레이어의 캐릭터 모델을 맵에서 숨김/무적 처리
		EscapedPlayer->SetActorHiddenInGame(true);
		EscapedPlayer->SetActorEnableCollision(false);

		// 5. 탈출한 플레이어 닉네임 가져오기 (스팀 닉네임 대응)
		FString EscaperName = TEXT("누군가");
		if (APlayerState* PS = PC->PlayerState)
		{
			EscaperName = PS->GetPlayerName();
		}

		// 6. 탈출하지 못한 다른 모든 플레이어에게 300의 데미지를 가해 사망하게 함
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* OtherPC = It->Get();
			if (OtherPC && OtherPC != PC)
			{
				if (AQPCharacter* OtherChar = Cast<AQPCharacter>(OtherPC->GetPawn()))
				{
					// 300 데미지 적용
					OtherChar->TakeDamage(300.f, FDamageEvent(), PC, EscapedPlayer);
				}
			}
		}

		// 7. 모든 플레이어(서버/클라이언트)에게 탈출 연출 메시지 브로드캐스트
		MulticastOnGameFinished(true, EscaperName);

		// 8. 5초 뒤 서버 트래블로 모든 세션 인원 메인 메뉴 이동
		FTimerHandle RestartTimerHandle;
		GetWorldTimerManager().SetTimer(RestartTimerHandle, [this]()
		{
			if (UWorld* World = GetWorld())
			{
				World->ServerTravel(TEXT("/Game/Maps/Main"));
			}
		}, 5.f, false);
	}
}

void AQPEscapeGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (bGameFinished)
	{
		// 라운드 종료(탈출 완료/실패) 상태에서는 더이상 캐릭터 리스폰을 하지 않음
		return;
	}
	Super::RequestRespawn(ElimmedCharacter, ElimmedController);
}

void AQPEscapeGameMode::MulticastOnGameFinished_Implementation(bool bVictory, const FString& WinnerNames)
{
	FString Msg;
	if (bVictory)
	{
		Msg = FString::Printf(TEXT("%s 이(가) 탈출에 성공하였습니다!\n5초 후 메인 메뉴로 이동합니다."), *WinnerNames);
	}
	else
	{
		Msg = TEXT("탈출 실패... 모든 생존자가 사망했습니다.\n5초 후 메인 메뉴로 이동합니다.");
	}

	// 모든 플레이어의 HUD에 메시지 출력
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			if (AQPCrosshair* HUD = Cast<AQPCrosshair>(PC->GetHUD()))
			{
				HUD->ShowBigMessage(Msg, 5.f);
			}
		}
	}
}
