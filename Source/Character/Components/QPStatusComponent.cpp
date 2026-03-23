#include "PJ_Quiet_Protocol/Character/Components/QPStatusComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Net/UnrealNetwork.h"

UQPStatusComponent::UQPStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UQPStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AQPCharacter>(GetOwner());

	// 서버에서 시작할 때 체력과 스테미나 초기화
	if (Character && Character->HasAuthority())
	{
		Health = MaxHealth;
		CurrentStamina = MaxStamina;
	}
	else
	{
		Health = MaxHealth;
		CurrentStamina = MaxStamina;
	}

	OnHealthChanged.Broadcast(Health / MaxHealth); // 초기 체력 상태 브로드캐스트
	OnStaminaChanged.Broadcast(CurrentStamina / MaxStamina); // 초기 스테미나 상태 브로드캐스트
}

void UQPStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character) return;

	if (Character->HasAuthority() || Character->IsLocallyControlled())
	{
		bool bStaminaChanged = false;

		// 달리는 중이면 스테미나 감소, 아니면 회복
		if (Character->IsSprinting())
		{
			if (CurrentStamina > 0.f)
			{
				CurrentStamina -= StaminaDrainRate * DeltaTime;
				if (CurrentStamina <= 0.f)
				{
					CurrentStamina = 0.f;
					bCanSprint = false;
					Character->StopSprint();
				}
				bStaminaChanged = true;
			}
			TimeSinceLastSprint = 0.f;
		}
		else 
		{
			TimeSinceLastSprint += DeltaTime;
			if (TimeSinceLastSprint >= StaminaRegenDelay && CurrentStamina < MaxStamina)
			{
				CurrentStamina += StaminaRegenRate * DeltaTime;
				if (CurrentStamina >= MaxStamina)
				{
					CurrentStamina = MaxStamina;
				}
				bStaminaChanged = true;

				if (!bCanSprint && CurrentStamina >= MaxStamina * MinStaminaPercentToSprint)
				{
					bCanSprint = true;
				}
			}
		}

		// 스테미나가 변경되었다면 이동속도 업데이트 및 UI 갱신 이벤트 브로드캐스트
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

	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth); // 체력 감소 후 클램프하여 0 이하로 떨어지지 않도록 함
	OnHealthChanged.Broadcast(Health / MaxHealth); // 체력 변경 이벤트 브로드캐스트

	if (Health <= 0.f)
	{
		Die();
	}
}

void UQPStatusComponent::Die()
{
	if (bIsDead) return;

	bIsDead = true;
	OnDeath.Broadcast(); // 사망 이벤트 브로드캐스트

	if (Character)
	{
		Character->Die(); 
	}
}

void UQPStatusComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health / MaxHealth); // 체력 변경이 클라이언트에 반영될 때마다 UI 갱신 이벤트 브로드캐스트
}

void UQPStatusComponent::OnRep_Stamina()
{
	if (Character)
	{
		Character->UpdateMovementSpeed(); // 클라이언트 측 이동속도 갱신
	}
	OnStaminaChanged.Broadcast(CurrentStamina / MaxStamina); // 스테미나 변경이 클라이언트에 반영될 때마다 UI 갱신 이벤트 브로드캐스트
}

void UQPStatusComponent::ServerUpdateStamina_Implementation(float NewStamina)
{
	CurrentStamina = NewStamina; // 서버에서 스테미나 업데이트 후 클라이언트에 반영
	if (CurrentStamina <= 0.f) // 스테미나가 0 이하로 떨어지면 달리기 불가능 상태로 전환
	{
		bCanSprint = false;
		if (Character) Character->StopSprint();
	}
	else if (CurrentStamina >= MaxStamina * MinStaminaPercentToSprint) // 스테미나가 충분히 회복되면 다시 달리기 가능 상태로 전환
	{
		bCanSprint = true;
	}
}
