#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QPStatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent); // 체력 변경 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath); // 사망 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent); // 스테미나 변경 이벤트 델리게이트

class AQPCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PJ_QUIET_PROTOCOL_API UQPStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UQPStatusComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ReceiveDamage(float DamageAmount);
	void Die();

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool CanSprint() const { return bCanSprint; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }
	
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }
	
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(Server, Reliable)
	void ServerUpdateStamina(float NewStamina);

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged; // 체력 변경 이벤트 델리게이트

	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaChanged OnStaminaChanged; // 스테미나 변경 이벤트 델리게이트

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath; // 사망 이벤트 델리게이트

	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f; // 최대 체력

	// Stamina
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float MaxStamina = 100.f; // 최대 스테미나

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ToolTip = "달릴 때 초당 감소하는 스테미나 양"))
	float StaminaDrainRate = 10.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ToolTip = "달리지 않을 때 초당 회복하는 스테미나 양"))
	float StaminaRegenRate = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ToolTip = "달리기를 멈춘 후 회복이 시작될 때까지의 지연 시간(초)"))
	float StaminaRegenDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ToolTip = "완전 고갈 시, 다시 달릴 수 있게 되기 위한 최소 스테미나 비율(0.0~1.0)"))
	float MinStaminaPercentToSprint = 0.3f; // 기본 30% 회복되면 다시 뛰기 가능

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Health();  // 체력 변경이 클라이언트에 반영될 때 호출되는 함수
	
	UFUNCTION()
	void OnRep_Stamina(); // 스테미나 변경이 클라이언트에 반영될 때 호출되는 함수

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Health")
	float Health; // 현재 체력

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Health")
	bool bIsDead = false; // 사망 여부

	UPROPERTY(ReplicatedUsing = OnRep_Stamina, VisibleAnywhere, Category = "Stamina")
	float CurrentStamina; // 현재 스테미나

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Stamina")
	bool bCanSprint = true; // 달릴 수 있는지 여부

	float TimeSinceLastSprint = 0.f; // 마지막으로 달린 후 경과 시간 추적

	UPROPERTY()
	AQPCharacter* Character;
};
