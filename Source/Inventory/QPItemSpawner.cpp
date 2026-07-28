#include "QPItemSpawner.h"
#include "WorldItemActor.h"
#include "ItemDataAsset.h"
#include "QPItemSpawnTypes.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "Engine/DataTable.h"

AQPItemSpawner::AQPItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 스포너 자체는 시각적 요소가 필요 없으며, 서버에서만 동작하고 사라집니다.
	bReplicates = false;

	// 에디터/빌드 환경에 관계없이 위치를 가질 수 있도록 항상 루트 컴포넌트를 생성합니다.
	USceneComponent* DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(DefaultRoot);
}

void AQPItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 아이템 생성을 처리합니다.
	if (HasAuthority())
	{
		SpawnRandomItem();
	}
}

void AQPItemSpawner::SpawnRandomItem()
{
	if (!DropTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AQPItemSpawner] DropTable is not assigned in %s"), *GetName());
		Destroy();
		return;
	}

	TArray<FItemSpawnRow*> Rows;
	DropTable->GetAllRows<FItemSpawnRow>(TEXT("ItemSpawner"), Rows);

	if (Rows.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AQPItemSpawner] DropTable is empty in %s"), *GetName());
		Destroy();
		return;
	}

	// 1. 총 가중치 합산
	float TotalWeight = 0.0f;
	for (const FItemSpawnRow* Row : Rows)
	{
		TotalWeight += Row->Weight;
	}

	if (TotalWeight <= 0.0f)
	{
		Destroy();
		return;
	}

	// 2. 랜덤 선택
	float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
	float CurrentWeight = 0.0f;
	FItemSpawnRow* SelectedRow = nullptr;

	for (FItemSpawnRow* Row : Rows)
	{
		CurrentWeight += Row->Weight;
		if (RandomValue <= CurrentWeight)
		{
			SelectedRow = Row;
			break;
		}
	}

	// 3. 아이템 스폰
	if (SelectedRow && !SelectedRow->ItemData.IsNull())
	{
		UItemDataAsset* ItemData = SelectedRow->ItemData.LoadSynchronous();
		if (ItemData)
		{
			FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeightOffset);
			FRotator SpawnRotation = GetActorRotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// 1순위: 행에서 직접 지정한 오버라이드 클래스 사용
			if (SelectedRow->ActorClassOverride)
			{
				AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SelectedRow->ActorClassOverride, SpawnLocation, SpawnRotation, SpawnParams);
				if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(SpawnedActor))
				{
					WorldItem->ItemData = ItemData;
					WorldItem->Quantity = FMath::RandRange(SelectedRow->MinQuantity, SelectedRow->MaxQuantity);
				}
			}
			// 2순위: 무기 타입인 경우 무기 클래스 직접 스폰
			else if (ItemData->ItemType == EItemType::EIT_Weapon && ItemData->WeaponClass)
			{
				GetWorld()->SpawnActor<AWeaponBase>(ItemData->WeaponClass, SpawnLocation, SpawnRotation, SpawnParams);
			}
			// 3순위: 기본 WorldItemActor 스폰
			else
			{
				TSubclassOf<AWorldItemActor> ClassToSpawn = WorldItemClass;
				if (!ClassToSpawn)
				{
					ClassToSpawn = AWorldItemActor::StaticClass();
				}
				AWorldItemActor* SpawnedItem = GetWorld()->SpawnActor<AWorldItemActor>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
				
				if (SpawnedItem)
				{
					SpawnedItem->ItemData = ItemData;
					SpawnedItem->Quantity = FMath::RandRange(SelectedRow->MinQuantity, SelectedRow->MaxQuantity);
				}
			}
		}
	}

	// 역할 완료 후 스포너 제거
	Destroy();
}
