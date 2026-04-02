#include "PJ_Quiet_Protocol/Character/Components/QPStatusComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Net/UnrealNetwork.h"

UQPStatusComponent::UQPStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 매 프레임 업데이트 활성화
	SetIsReplicatedByDefault(true); // 컴포넌트 리플리케이션 활성화
}

void UQPStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AQPCharacter>(GetOwner());

	// 서버 권한이 있는 경우 초기 상태값 설정
	if (Character && Character->HasAuthority())
	{
		Health = MaxHealth;
		CurrentStamina = MaxStamina;
	}
	else
	{
		// 클라이언트에서도 로컬 예측을 위해 초기값 설정
		Health = MaxHealth;
		CurrentStamina = MaxStamina;
	}

	// UI 업데이트를 위한 초기 방송(Broadcast)
	OnHealthChanged.Broadcast(Health / MaxHealth);
	OnStaminaChanged.Broadcast(CurrentStamina / MaxStamina);
}

void UQPStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character) return;

	// 서버 또는 로컬 플레이어 직접 제어 시 스태미나 계산 수행
	if (Character->HasAuthority() || Character->IsLocallyControlled())
	{
		bool bStaminaChanged = false;
		
		// 스프린트(질주) 중일 때 스태미나 소모
		if (Character->IsSprinting())
		{
			if (CurrentStamina > 0.f)
			{
				CurrentStamina -= StaminaDrainRate * DeltaTime;
				if (CurrentStamina <= 0.f)
				{
					CurrentStamina = 0.f;
					bCanSprint = false; // 스태미나 고갈 시 질주 불가 상태로 전환
					Character->StopSprint();
				}
				bStaminaChanged = true;
			}
			TimeSinceLastSprint = 0.f; // 마지막 질주 시간 초기화
		}
		else 
		{
			// 질주 중이 아닐 때 스태미나 회복 로직
			TimeSinceLastSprint += DeltaTime;
			// 일정 지연 시간(RegenDelay) 이후부터 회복 시작
			if (TimeSinceLastSprint >= StaminaRegenDelay && CurrentStamina < MaxStamina)
			{
				CurrentStamina += StaminaRegenRate * DeltaTime;
				if (CurrentStamina >= MaxStamina)
				{
					CurrentStamina = MaxStamina;
				}
				bStaminaChanged = true;

				// 최소 요구치 이상 회복되면 다시 질주 가능 상태로 전환
				if (!bCanSprint && CurrentStamina >= MaxStamina * MinStaminaPercentToSprint)
				{
					bCanSprint = true;
				}
			}
		}

		// 스태미나 수치가 변경되었다면 속도 업데이트 및 UI 델리게이트 호출
		if (bStaminaChanged)
		{
			Character->UpdateMovementSpeed();
			OnStaminaChanged.Broadcast(CurrentStamina / MaxStamina);
		}
	}
}

void UQPStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UQPStatusComponent, Health);
	DOREPLIFETIME(UQPStatusComponent, bIsDead);
	DOREPLIFETIME(UQPStatusComponent, CurrentStamina);
	DOREPLIFETIME(UQPStatusComponent, bCanSprint);
}

void UQPStatusComponent::ReceiveDamage(float DamageAmount)
{
	if (bIsDead) return;

	// 데미지 적용 및 체력 범위 클램핑 (0 ~ MaxHealth)
	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health / MaxHealth);

	// 체력이 0 이하가 되면 사망 처리
	if (Health <= 0.f)
	{
		Die();
	}
}

void UQPStatusComponent::Die()
{
	if (bIsDead) return;

	bIsDead = true;
	OnDeath.Broadcast();

	if (Character)
	{
		Character->Die(); 
	}
}

void UQPStatusComponent::OnRep_Health()
{
	// 서버로부터 복제된 체력 값이 변경되었을 때 클라이언트 UI 업데이트
	OnHealthChanged.Broadcast(Health / MaxHealth);
}

void UQPStatusComponent::OnRep_Stamina()
{
	// 서버로부터 복제된 스태미나 값이 변경되었을 때 관련 로직 동기화
	if (Character)
	{
		Character->UpdateMovementSpeed();
	}
	OnStaminaChanged.Broadcast(CurrentStamina / MaxStamina);
}

void UQPStatusComponent::ServerUpdateStamina_Implementation(float NewStamina)
{
	CurrentStamina = NewStamina;
	if (CurrentStamina <= 0.f)
	{
		bCanSprint = false;
		if (Character) Character->StopSprint();
	}
	else if (CurrentStamina >= MaxStamina * MinStaminaPercentToSprint)
	{
		bCanSprint = true;
	}
}
