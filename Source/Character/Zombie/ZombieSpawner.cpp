#include "ZombieSpawner.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ZombieCharacter.h"

AZombieSpawner::AZombieSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SetRootComponent(SpawnArea);
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnArea->SetBoxExtent(FVector(500.f, 500.f, 200.f));


}

void AZombieSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStart) StartSpawning();
}

void AZombieSpawner::StartSpawning()
{
	if (!GetWorld()) return;
	if (GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle)) return; //이미 스폰 중이면 반환

	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AZombieSpawner::TickSpawn, SpawnInterval, true); //스폰 타이머 설정
}
void AZombieSpawner::StopSpawning()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle); //스폰 타이머 정리
}

void AZombieSpawner::CleanupDeadRefs()
{
	AliveZombies.RemoveAll([](const TObjectPtr<AZombieCharacter>& Zombie)
		{
			return (Zombie == nullptr) || Zombie->IsActorBeingDestroyed();
		});
}

void AZombieSpawner::TickSpawn()
{
	CleanupDeadRefs();

	if (MaxAlive <= 0) return;
	if (AliveZombies.Num() >= MaxAlive) return;

	const TSubclassOf<AZombieCharacter> PickedClass = PickZombieClassWeighted();
	if (!PickedClass) return;

	FVector SpawnLocation;
	if (!FindSpawnLocation(SpawnLocation)) return;

	FRotator SpawnRotation = GetActorRotation();
	SpawnRotation = FRotator(0.f, UKismetMathLibrary::RandomFloatInRange(0.f, 360.f), 0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AZombieCharacter* SpawnedZombie = GetWorld()->SpawnActor<AZombieCharacter>(PickedClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!SpawnedZombie) return;

	AliveZombies.Add(SpawnedZombie);

	SpawnedZombie->OnDestroyed.AddDynamic(this, &AZombieSpawner::HandleSpawnedActorDestroyed);
}

void AZombieSpawner::HandleSpawnedActorDestroyed(AActor* DestroyedActor)
{
	AZombieCharacter* DestroyedZombie = Cast<AZombieCharacter>(DestroyedActor);
	if (!DestroyedZombie) return;
	AliveZombies.Remove(DestroyedZombie);
}

TSubclassOf<AZombieCharacter> AZombieSpawner::PickZombieClassWeighted() const
{
	// 가중치(Weight) 기반 확률 스폰 로직
	float TotalWeight = 0.f;
	for (const FZombieSpawnEntry& Entry : SpawnList)
	{
		if (Entry.ZombieClass && Entry.Weight > 0.f)
		{
			TotalWeight += Entry.Weight;
		}
	}
	if (TotalWeight <= 0.f) return nullptr;

	float Range = FMath::FRandRange(0.f, TotalWeight);
	for (const FZombieSpawnEntry& Entry : SpawnList)
	{
		if (!Entry.ZombieClass || Entry.Weight <= 0.f) continue;
		Range -= Entry.Weight;
		if (Range <= 0.f)
		{
			return Entry.ZombieClass;
		}
	}
	for (const FZombieSpawnEntry& Entry : SpawnList)
	{
		if (Entry.ZombieClass) return Entry.ZombieClass;
	}
	return nullptr;
}

bool AZombieSpawner::FindSpawnLocation(FVector& OutLocation) const
{
	if (!GetWorld()) return false;

	const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	if (!NavSys) return false;

	const FVector BoxOrigin = SpawnArea->GetComponentLocation();
	const FVector BoxExtent = SpawnArea->GetScaledBoxExtent();
	const FVector RandomPointInBox = UKismetMathLibrary::RandomPointInBoundingBox(BoxOrigin, BoxExtent);

	FNavLocation NavLocation;
	const bool bFoundNavLocation = NavSys->GetRandomPointInNavigableRadius(RandomPointInBox, NavSearchRadius, NavLocation);

	if (!bFoundNavLocation) return false;
	OutLocation = NavLocation.Location;
	return true;
}