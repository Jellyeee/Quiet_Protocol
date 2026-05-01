#include "PJ_Quiet_Protocol/Environment/QPEscapeDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameMode.h"
#include "PJ_Quiet_Protocol/UserWidget/Crosshair/QPCrosshair.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Character/Zombie/ZombieCharacter.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"
AQPEscapeDoor::AQPEscapeDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 시작 시에는 Tick을 끔
	bReplicates = true;

	DoorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorVolume"));
	SetRootComponent(DoorVolume);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(RootComponent);

	EscapeZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("EscapeZoneVolume"));
	EscapeZoneVolume->SetupAttachment(RootComponent);
	EscapeZoneVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 닫혀있을 땐 비활성

	bIsRealExit = false;
	bIsOpening = false;
	SpawnedZombieCount = 0;
	CurrentInterpolation = 0.f;
}

void AQPEscapeDoor::BeginPlay()
{
	Super::BeginPlay();

	// 시작 위치 저장 및 목표 위치 계산
	StartLocation = DoorMesh->GetRelativeLocation();
	TargetLocation = StartLocation + FVector(0.f, 0.f, OpenHeight);

	if (HasAuthority())
	{
		EscapeZoneVolume->OnComponentBeginOverlap.AddDynamic(this, &AQPEscapeDoor::OnEscapeZoneOverlap);
	}
}

void AQPEscapeDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsOpening)
	{
		CurrentInterpolation += DeltaTime * OpenSpeed;
		
		if (CurrentInterpolation >= 1.f)
		{
			CurrentInterpolation = 1.f;
			bIsOpening = false;
			SetActorTickEnabled(false); // 이동 완료 후 Tick 다시 끔


			// 문이 완전히 열렸으므로 진짜 출구일 경우에만 탈출 구역 활성화
			if (bIsRealExit && HasAuthority())
			{
				EscapeZoneVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}

		// Lerp를 이용해 부드럽게 이동
		FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, CurrentInterpolation);
		DoorMesh->SetRelativeLocation(NewLocation);
	}
}

void AQPEscapeDoor::SubmitPassword_Implementation(const TArray<int32>& InputPassword)
{
	if (AQPEscapeGameMode* GM = GetWorld()->GetAuthGameMode<AQPEscapeGameMode>())
	{
		// GM->VerifyEscapeAttempt 내부에서 정답/오답/진짜/가짜 여부에 따른 성공(Open) 또는 함정(Trap/Aggro) 로직이 직접 실행됨
		GM->VerifyEscapeAttempt(InputPassword, this);
	}
}

void AQPEscapeDoor::OpenDoor()
{
	if (!HasAuthority()) return;

	MulticastOpenDoor();
}

void AQPEscapeDoor::TriggerTrap()
{
	if (!HasAuthority()) return;

	if (TrapZombieClass && TrapZombieCount > 0)
	{
		SpawnedZombieCount = 0;
		GetWorld()->GetTimerManager().SetTimer(TrapTimerHandle, this, &AQPEscapeDoor::SpawnTrapZombie, TrapZombieSpawnInterval, true, 0.f);
	}

	MulticastTriggerTrap();
}

void AQPEscapeDoor::MulticastOpenDoor_Implementation()
{
	// 문 열리는 시각/청각 효과


	// 애니메이션 시작
	bIsOpening = true;
	CurrentInterpolation = 0.f;
	SetActorTickEnabled(true);
}

void AQPEscapeDoor::MulticastTriggerTrap_Implementation()
{
	// 가짜 문 개방


	bIsOpening = true;
	CurrentInterpolation = 0.f;
	SetActorTickEnabled(true);
}

void AQPEscapeDoor::TriggerWrongPasswordAggro()
{
	if (!HasAuthority()) return;
	MulticastTriggerWrongPasswordAggro();
}

void AQPEscapeDoor::MulticastTriggerWrongPasswordAggro_Implementation()
{


	// 오디오 뼈대 재생
	if (WrongPasswordSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WrongPasswordSound, GetActorLocation());
	}

	// 서버 권한인 경우 주변 AI(좀비)에게 거대한 소음을 전파
	if (HasAuthority())
	{
		// 뛰는소리(1.x), 총소리(2~4), 경보어그로(15.f)
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 15.0f, this, 0.f, TEXT("AlarmNoise"));
	}
}

void AQPEscapeDoor::SpawnTrapZombie()
{
	if (!HasAuthority()) return;

	if (SpawnedZombieCount >= TrapZombieCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(TrapTimerHandle);
		return;
	}

	// 성능 보호 (MaxGlobalZombies 초과 시 스폰 스킵)
	TArray<AActor*> AllZombies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AZombieCharacter::StaticClass(), AllZombies);
	if (AllZombies.Num() >= MaxGlobalZombies)
	{
		// 스폰 제한에 걸리면 타이머 종료 혹은 대기할 수 있으나, 현재는 그냥 이번 틱 스킵 후 종료
		GetWorld()->GetTimerManager().ClearTimer(TrapTimerHandle);
		return;
	}

	// 문의 뒷쪽(-Forward)에 스폰
	FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * -200.f) + FVector(0.f, 0.f, 20.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (GetWorld()->SpawnActor<ACharacter>(TrapZombieClass, SpawnLoc, GetActorRotation(), SpawnParams))
	{
		SpawnedZombieCount++;
	}
}

void AQPEscapeDoor::OnEscapeZoneOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		if (AQPCharacter* Character = Cast<AQPCharacter>(OtherActor))
		{
			// 플레이어가 탈출 구역에 진입 시 클리어 트리거
			if (AQPEscapeGameMode* GM = GetWorld()->GetAuthGameMode<AQPEscapeGameMode>())
			{
				GM->OnPlayerEscaped(Character);
			}
		}
	}
}

void AQPEscapeDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQPEscapeDoor, bIsRealExit);
}
