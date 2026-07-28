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

	// 큰 메시지 출력 (탈출 성공/함정 발동 등) - Pawn 유무와 관계없이 실행되도록 상단 배치
	if (MessageTimer > 0.f)
	{
		MessageTimer -= GetWorld()->GetDeltaSeconds();

		float TextX = ViewportCenter.X;
		float TextY = ViewportCenter.Y - 200.f; // 중앙보다 약간 위

		if (GEngine && GEngine->GetLargeFont())
		{
			float FontScale = 3.5f;
			// 텍스트 중앙 정렬을 위한 오프셋 (텍스트 길이에 따라 유동적)
			DrawText(CurrentMessage, CurrentMessageColor, TextX - (CurrentMessage.Len() * 20.f), TextY, GEngine->GetLargeFont(), FontScale, false);
		}
	}

	APawn* Pawn = GetOwningPawn();
	if (Pawn)
	{
		AQPCharacter* Character = Cast<AQPCharacter>(Pawn);
		if (Character && Character->IsDead())
		{
			return; // 사망 시 크로스헤어 비활성화
		}

		// [Change] 자체 계산 대신 CombatComponent에서 이미 계산된 확산 값을 가져와 동기화함
		if (UQPCombatComponent* Combat = Character->FindComponentByClass<UQPCombatComponent>())
		{
			CrosshairSpread = Combat->GetCrosshairSpread();
			CrosshairScreenOffset = Combat->GetCrosshairScreenOffset();
		}
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
				float ShieldPercent = FMath::Clamp(Status->GetShield() / FMath::Max(1.f, Status->GetMaxShield()), 0.f, 1.f);
				float StaminaPercent = FMath::Clamp(Status->GetCurrentStamina() / FMath::Max(1.f, Status->GetMaxStamina()), 0.f, 1.f);

				float BarWidth = 300.f;
				float BarHeight = 15.f;
				float StartX = ViewportSize.X / 2.f - (BarWidth / 2.f);
				float StartY = ViewportSize.Y - 120.f; // 중앙 하단

				// 체력 게이지 (회색) 배경 및 전경
				DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f), StartX, StartY, BarWidth, BarHeight);
				DrawRect(FLinearColor(0.5f, 0.5f, 0.5f, 1.f), StartX, StartY, BarWidth * HealthPercent, BarHeight);

				// 보호막 게이지 (파란색) - 체력바 바로 아래 가느다랗게 추가
				if (Status->GetShield() > 0.f)
				{
					float ShieldBarHeight = BarHeight * 0.4f; // 얇은 높이
					float ShieldStartY = StartY + BarHeight + 2.f; // 체력바 바로 아래
					DrawRect(FLinearColor(0.05f, 0.1f, 0.2f, 0.6f), StartX, ShieldStartY, BarWidth, ShieldBarHeight);
					DrawRect(FLinearColor(0.0f, 0.6f, 1.0f, 1.f), StartX, ShieldStartY, BarWidth * ShieldPercent, ShieldBarHeight);
				}

				// 스태미나 게이지 (회색 + 약간의 하늘색) 배경 및 전경
				// 보호막이 생길 수 있으므로 간격을 조금 더 둠
				float StaminaY = StartY + BarHeight + 18.f;
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

void AQPCrosshair::ShowBigMessage(const FString& Message, float Duration)
{
	CurrentMessage = Message;
	MessageTimer = Duration;

	// 기본 색상을 하얀색으로 설정
	CurrentMessageColor = FLinearColor::White;

	// 메시지 내용에 따라 색상 지정 (한글 인코딩 문제를 고려해 최대한 단순하게 매칭)
	if (Message.Contains(TEXT("성공")) || Message.Contains(TEXT("Success")))
	{
		CurrentMessageColor = FLinearColor::Green; 
	}
	else if (Message.Contains(TEXT("함정")) || Message.Contains(TEXT("발동")) || Message.Contains(TEXT("Trap")))
	{
		CurrentMessageColor = FLinearColor::Red;
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
