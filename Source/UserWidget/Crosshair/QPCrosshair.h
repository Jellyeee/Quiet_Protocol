// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "QPCrosshair.generated.h"

/**
 * 
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPCrosshair : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

	// 크로스헤어 텍스처와 관련된 변수
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairTexture;

	// 크로스헤어의 위치를 화면 중앙에서 얼마나 떨어뜨릴지 결정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FVector2D CrosshairScreenOffset = FVector2D(0.f, 0.f); // 화면 중앙으로부터의 오프셋 (양수: 우/하)

	// 크로스헤어의 각 부분을 나타내는 텍스처 변수들
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	class UTexture2D* CrosshairBottom;

	// 동적 확산 (Dynamic Spread) 관련 변수
	float CrosshairSpread;
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairSpreadBase = 0.f; // 기본 확산 값

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairVelocityFactor = 2.f; // 이동 속도에 따른 확산 계수

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairInAirFactor = 2.f; // 공중 상태일 때 확산 계수

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairAimFactor = 0.f; // 조준 시 확산 감소 계수 

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairShootingFactor = 2.f; // 사격 시 확산 증가 계수 


	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowBigMessage(const FString& Message, float Duration = 3.f);

private:
	void DrawCrosshairPart(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread); // 각 크로스헤어 부분을 그리는 함수
	
	FString CurrentMessage;
	float MessageTimer = 0.f;
	FLinearColor CurrentMessageColor = FLinearColor::White;
};
