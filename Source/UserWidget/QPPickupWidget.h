// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QPPickupWidget.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPPickupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Pickup")
	void SetTargetActor(AActor* NewTarget) { TargetActor = NewTarget; } // 타겟 액터 설정 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Pickup")
	AActor* GetTargetActor() const { return TargetActor; } // 타겟 액터 가져오기 함수
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; // 네이티브 틱 함수 오버라이드
	UPROPERTY(BlueprintReadOnly, Category = "UI|Pickup")
	TObjectPtr<AActor> TargetActor = nullptr; // 타겟 액터 포인터

};
