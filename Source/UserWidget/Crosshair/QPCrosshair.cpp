// Fill out your copyright notice in the Description page of Project Settings.


#include "QPCrosshair.h"
#include "Engine/Canvas.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "GameFramework/Pawn.h" 
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PJ_Quiet_Protocol/Weapons/GunWeapon.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "PJ_Quiet_Protocol/Character/Components/QPStatusComponent.h"

void AQPCrosshair::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize = FVector2D::ZeroVector; // [Fix] C4701: 초기화되지 않은 변수 경고 수정
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); 
	ViewportCenter += CrosshairScreenOffset; // 화면 중앙에서의 오프셋 적용
	FVector2D Spread(0.f, 0.f); // 확산 값 초기화

	APawn* Pawn = GetOwningPawn();
	if (Pawn)
	{
		AQPCharacter* Character = Cast<AQPCharacter>(Pawn);
		if (Character && Character->IsDead())
		{
			return; // 사망 시 크로스헤어 비활성화
		}

		FVector Velocity = Pawn->GetVelocity(); 
		Velocity.Z = 0.f; // 수직 속도는 확산 계산에서 제외 (수평 이동만 고려)
		float Speed = Velocity.Size(); // 이동 속도 계산

		float VelocitySpread = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 600.f), FVector2D(0.f, CrosshairSpreadMax), Speed); // 이동 속도에 따른 확산 계산
		
		float InAirSpread = 0.f; // 공중 상태일 때 확산 계산
		if (Pawn->GetMovementComponent() && Pawn->GetMovementComponent()->IsFalling()) // 캐릭터가 공중에 있는지 확인
		{
			InAirSpread = CrosshairSpreadMax * 2.f;  
		}

		float AimingSpread = 0.f; // 조준 시 확산 감소 계산
		if (Character) 
		{
			if (Character->IsAiming())
			{
				VelocitySpread *= 0.1f; 
				InAirSpread *= 0.1f;    
			}
		}

		float TargetSpread = CrosshairSpreadBase + VelocitySpread + InAirSpread; 

		float DeltaTime = GetWorld()->GetDeltaSeconds(); 
		CrosshairSpread = FMath::FInterpTo(CrosshairSpread, TargetSpread, DeltaTime, 10.f);
	}

	Spread.X = CrosshairSpread;
	Spread.Y = CrosshairSpread;

	// 각 크로스헤어 부분을 그리는 함수 호출 (중앙, 왼쪽, 오른쪽, 위, 아래)
	if (CrosshairCenter)
	{
		FVector2D SpreadNone(0.f, 0.f);
		DrawCrosshairPart(CrosshairCenter, ViewportCenter, SpreadNone);
	}
	if (CrosshairLeft)
	{
		FVector2D SpreadLeft(-Spread.X, 0.f);
		DrawCrosshairPart(CrosshairLeft, ViewportCenter, SpreadLeft);
	}
	if (CrosshairRight)
	{
		FVector2D SpreadRight(Spread.X, 0.f);
		DrawCrosshairPart(CrosshairRight, ViewportCenter, SpreadRight);
	}
	if (CrosshairTop)
	{
		FVector2D SpreadTop(0.f, -Spread.Y);
		DrawCrosshairPart(CrosshairTop, ViewportCenter, SpreadTop);
	}
	if (CrosshairBottom)
	{
		FVector2D SpreadBottom(0.f, Spread.Y);
		DrawCrosshairPart(CrosshairBottom, ViewportCenter, SpreadBottom);
	}

	// 현재 장착된 무기의 탄약수 표시
	if (Pawn)
	{
		AQPCharacter* Character = Cast<AQPCharacter>(Pawn);
		if (Character && !Character->IsDead())
		{
			UQPCombatComponent* Combat = Character->FindComponentByClass<UQPCombatComponent>();
			UInventoryComponent* Inventory = Character->FindComponentByClass<UInventoryComponent>();
			UQPStatusComponent* Status = Character->FindComponentByClass<UQPStatusComponent>();

			if (Status)
			{
				float HealthPercent = FMath::Clamp(Status->GetHealth() / FMath::Max(1.f, Status->GetMaxHealth()), 0.f, 1.f);
				float StaminaPercent = FMath::Clamp(Status->GetCurrentStamina() / FMath::Max(1.f, Status->GetMaxStamina()), 0.f, 1.f);

				float BarWidth = 300.f;
				float BarHeight = 15.f;
				float StartX = ViewportCenter.X - (BarWidth / 2.f);
				float StartY = ViewportSize.Y - 120.f; // 중앙 하단

				// 체력 게이지 (회색) 배경 및 전경
				DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f), StartX, StartY, BarWidth, BarHeight);
				DrawRect(FLinearColor(0.5f, 0.5f, 0.5f, 1.f), StartX, StartY, BarWidth * HealthPercent, BarHeight);

				// 스태미나 게이지 (회색 + 약간의 하늘색) 배경 및 전경
				float StaminaY = StartY + BarHeight + 10.f;
				DrawRect(FLinearColor(0.1f, 0.15f, 0.2f, 0.6f), StartX, StaminaY, BarWidth, BarHeight);
				DrawRect(FLinearColor(0.4f, 0.6f, 0.7f, 1.f), StartX, StaminaY, BarWidth * StaminaPercent, BarHeight); 
			}

			if (Combat && Combat->HasWeapon())
			{
				if (AGunWeapon* Gun = Cast<AGunWeapon>(Combat->GetEquippedWeapon()))
				{
					int32 TotalAmmo = 0;
					if (Inventory)
					{
						TotalAmmo = Inventory->GetTotalAmmo(Combat->GetEquippedWeaponType());
					}
					
					FString AmmoText = FString::Printf(TEXT("%d / %d"), Gun->GetCurrentAmmo(), TotalAmmo);
					float TextX = ViewportSize.X * 0.85f;
					float TextY = ViewportSize.Y * 0.9f;
					
					if (GEngine && GEngine->GetLargeFont())
					{
						DrawText(AmmoText, FLinearColor::White, TextX, TextY, GEngine->GetLargeFont(), 2.0f, false);
					}
					else
					{
						DrawText(AmmoText, FLinearColor::White, TextX, TextY);
					}
				}
			}
		}
	}
}


void AQPCrosshair::DrawCrosshairPart(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread) // 각 크로스헤어 부분을 그리는 함수
{
	float TextureWidth = Texture->GetSurfaceWidth(); // 텍스처의 너비와 높이를 가져옴
	float TextureHeight = Texture->GetSurfaceHeight(); // 텍스처의 너비와 높이를 가져옴
	FVector2D CrosshairDrawPoint( 
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	); // 크로스헤어를 그릴 위치 계산 (화면 중앙에서 텍스처의 절반 크기만큼 빼고, 확산 값 추가)

	DrawTexture(
		Texture,
		CrosshairDrawPoint.X,
		CrosshairDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		FLinearColor::White
	); // 텍스처를 화면에 그리는 함수 호출 (텍스처, 위치, 크기, UV 좌표, 색상 등)
}
