#include "ZombieSpawner.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ZombieCharacter.h"

AZombieSpawner::AZombieSpawner()
{
	PrimaryActorTick.bCanEverTick = false; //틱 비활성화
	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea")); //스폰	영역 박스 컴포넌트 생성
	SetRootComponent(SpawnArea); //루트 컴포넌트로 설정
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 비활성화
	SpawnArea->SetBoxExtent(FVector(500.f, 500.f, 200.f)); //기본 스폰 박스 크기 설정


}

void AZombieSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStart) StartSpawning(); //자동 시작 설정 시 스폰 시작
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

void AZombieSpawner::CleanupDeadRefs() //죽은 좀비 참조 정리 함수
{
	AliveZombies.RemoveAll([](const TObjectPtr<AZombieCharacter>& Zombie) //람다 함수로 죽은 좀비 참조 제거
		{
			return (Zombie == nullptr) || Zombie->IsActorBeingDestroyed(); //좀비가 널이거나 파괴 중인지 확인
		});
}

void AZombieSpawner::TickSpawn() //스폰 틱 함수
{
	CleanupDeadRefs(); //죽은 좀비 참조 정리

	if (MaxAlive <= 0) return; //최대 생존 좀비 수가 0 이하이면 반환
	if (AliveZombies.Num() >= MaxAlive) return; //현재 생존 좀비

	const TSubclassOf<AZombieCharacter> PickedClass = PickZombieClassWeighted(); //가중치 기반 좀비 클래스 선택
	if (!PickedClass) return; //선택된 클래스가 없으면 반환

	FVector SpawnLocation; //스폰 위치 변수
	if (!FindSpawnLocation(SpawnLocation)) return; //스폰 위치 찾기 실패 시

	FRotator SpawnRotation = GetActorRotation(); //스폰 회전 설정
	SpawnRotation = FRotator(0.f, UKismetMathLibrary::RandomFloatInRange(0.f, 360.f), 0.f); //랜덤한 Yaw 회전 설정

	FActorSpawnParameters SpawnParams; //스폰 파라미터 설정
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; //충돌 처리 방법 설정
	AZombieCharacter* SpawnedZombie = GetWorld()->SpawnActor<AZombieCharacter>(PickedClass, SpawnLocation, SpawnRotation, SpawnParams); //좀비 스폰

	if (!SpawnedZombie) return;

	AliveZombies.Add(SpawnedZombie); //생존 좀비 목록에 추가

	SpawnedZombie->OnDestroyed.AddDynamic(this, &AZombieSpawner::HandleSpawnedActorDestroyed); //좀비가 파괴되면 Alive 리스트에서 제거되도록 연결
}

void AZombieSpawner::HandleSpawnedActorDestroyed(AActor* DestroyedActor) //좀비 파괴 핸들러
{
	AZombieCharacter* DestroyedZombie = Cast<AZombieCharacter>(DestroyedActor); //파괴된 액터를 좀비로 캐스팅
	if (!DestroyedZombie) return; //캐스팅 실패 시 반환
	AliveZombies.Remove(DestroyedZombie); //생존 좀비 목록에서 제거
}

TSubclassOf<AZombieCharacter> AZombieSpawner::PickZombieClassWeighted() const //가중치 기반 좀비 클래스 선택 함수
{
	//가중치(Weight)는 각 좀비 타입이 스폰될 확률을 결정하는 숫자
	//예를 들어 일반 좀비 가중치 7, 강한 좀비 가중치 3이면 → 총합 10 중에서 일반 좀비는 70 %, 강한 좀비는 30 % 확률로 스폰됨
	//가중치가 높을수록 해당 좀비가 더 자주 등장하고, 0이면 스폰되지 않음.
	float TotalWeight = 0.f; //총 가중치 계산
	for (const FZombieSpawnEntry& Entry : SpawnList) //스폰 목록 순회
	{
		if (Entry.ZombieClass && Entry.Weight > 0.f) //유효한 클래스와 가중치인 경우
		{
			TotalWeight += Entry.Weight;  //가중치 누적
		}
	}
	if (TotalWeight <= 0.f) return nullptr; //총 가중치가 0 이하이면 반환

	float Range = FMath::FRandRange(0.f, TotalWeight); //0부터 총 가중치 사이의 랜덤 값 생성
	for (const FZombieSpawnEntry& Entry : SpawnList) //스폰 목록 순회
	{
		if (!Entry.ZombieClass || Entry.Weight <= 0.f) continue; //유효하지 않은 항목 건너뜀
		Range -= Entry.Weight; //랜덤 값에서 가중치 차감
		if (Range <= 0.f) //랜덤 값이 0 이하가 되면
		{
			return Entry.ZombieClass; //해당 좀비 클래스 반환
		}
	}
	for (const FZombieSpawnEntry& Entry : SpawnList) //안전망: 유효한 첫 번째 좀비 클래스 반환
	{
		if (Entry.ZombieClass) return Entry.ZombieClass; //유효한 클래스 반환
	}
	return nullptr; //없으면 널 반환
}

bool AZombieSpawner::FindSpawnLocation(FVector& OutLocation) const //스폰 위치 찾기 함수
{
	if (!GetWorld()) return false; //월드 유효성 검사

	const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()); //네비게이션 시스템 가져오기

	if (!NavSys) return false; //네비게이션 시스템 유효성 검사

	// 1) 박스 범위 안에서 랜덤 점을 하나 뽑고
	const FVector BoxOrigin = SpawnArea->GetComponentLocation(); //박스 중심 위치
	const FVector BoxExtent = SpawnArea->GetScaledBoxExtent(); //박스 크기
	const FVector RandomPointInBox = UKismetMathLibrary::RandomPointInBoundingBox(BoxOrigin, BoxExtent); //박스 내 랜덤 점

	// 2) 그 근처	에서 네비게이션 위치를 찾음
	FNavLocation NavLocation; //네비게이션 위치 변수
	const bool bFoundNavLocation = NavSys->GetRandomPointInNavigableRadius(RandomPointInBox, NavSearchRadius, NavLocation); //네비게이션 위치 찾기

	if (!bFoundNavLocation) return false; //네비게이션 위치 찾기 실패 시 반환
	OutLocation = NavLocation.Location; //출력 위치 설정
	return true; //성공 반환
}