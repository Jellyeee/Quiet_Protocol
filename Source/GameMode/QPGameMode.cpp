// Fill out your copyright notice in the Description page of Project Settings.

#include "PJ_Quiet_Protocol/GameMode/QPGameMode.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void AQPGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// 10초 후에 기존 시체 제거 및 랜덤 지정된 스폰 위치에서 캐릭터 재생성
		FTimerHandle RespawnTimer;
		
		// 람다에서 안전하게 참조하기 위해 Weak 폰인터 사용
		TWeakObjectPtr<ACharacter> WeakCharacter(ElimmedCharacter);
		TWeakObjectPtr<AController> WeakController(ElimmedController);

		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindLambda([this, WeakCharacter, WeakController]()
		{
			// 1. 기존 시체 충돌 해제 및 제거를 먼저 수행하여 스폰 위치가 막히는 것을 방지
			if (WeakCharacter.IsValid())
			{
				WeakCharacter->SetActorHiddenInGame(true);
				WeakCharacter->SetActorEnableCollision(false);
				WeakCharacter->Destroy();
			}

			// 2. 컨트롤러가 유효하면 플레이어 부활 처리
			if (WeakController.IsValid())
			{
				AController* Controller = WeakController.Get();
				
				// 혹시 아직 빙의 해제가 안 되었다면 강제로 해제
				if (Controller->GetPawn())
				{
					Controller->UnPossess();
				}

				TArray<AActor*> PlayerStarts;
				UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

				AActor* SpawnPoint = nullptr;
				if (PlayerStarts.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, PlayerStarts.Num() - 1);
					SpawnPoint = PlayerStarts[RandomIndex];
				}

				if (SpawnPoint)
				{
					RestartPlayerAtPlayerStart(Controller, SpawnPoint);
				}
				else
				{
					RestartPlayer(Controller); // 없으면 기본 Restart 호출
				}
			}
		});

		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, RespawnDelegate, 10.f, false); // 10초 대기 타이머
	}
}
