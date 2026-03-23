#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawner.generated.h"

USTRUCT(BlueprintType)
struct FZombieSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<class AZombieCharacter> ZombieClass; //좀비 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float Weight = 1.0f; //스폰 가중치
};




UCLASS()
class PJ_QUIET_PROTOCOL_API AZombieSpawner : public AActor
{
	GENERATED_BODY()

public:
	AZombieSpawner();

protected:
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr <class UBoxComponent> SpawnArea; //스폰 영역 박스 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FZombieSpawnEntry> SpawnList; //스폰할 좀비 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	int32 MaxAlive = 10; //최대 생존 좀비 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float SpawnInterval = 10.f; //스폰 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float NavSearchRadius = 500.f; //네비게이션 검색 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	bool bAutoStart = true; //자동 시작 여부

public:
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void StartSpawning(); //스폰 시작 함수
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void StopSpawning(); //스폰 중지 함수

private:
	FTimerHandle SpawnTimerHandle; //스폰 타이머 핸들
	UPROPERTY()
	TArray<TObjectPtr<class AZombieCharacter>> AliveZombies; //생존 중인 좀비 목록
	void TickSpawn(); //스폰 틱 함수
	void CleanupDeadRefs(); //죽은 좀비 참조 정리 함수
	TSubclassOf<class AZombieCharacter> PickZombieClassWeighted() const; //가중치 기반 좀비 클래스 선택 함수
	bool FindSpawnLocation(FVector& OutLocation) const; //스폰 위치 찾기 함수
	UFUNCTION()
	void HandleSpawnedActorDestroyed(AActor* DestroyedActor); //스폰된 액터 파괴 핸들러


};