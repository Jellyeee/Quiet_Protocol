// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "QPPlayerController.generated.h"

/**
 *
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override; 
	//Pickup Target Setting Function
	UFUNCTION(BlueprintCallable, Category = "UI|Pickup")
	void SetPickupTarget(AActor* NewTarget); // 픽업 타겟 설정 함수
	void ToggleInventory(); // 인벤토리 토글 함수
	void ToggleLootInventory(); // 전리품 인벤토리 토글 함수

	// 일시정지 메뉴 토글
	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void TogglePauseMenu();

	/** 모든 게임플레이 UI를 강제로 닫고 입력 모드를 초기화합니다. (사망 시 호출) */
	void ClearAllUI();

protected:
	//Pickup Widget Class
	UPROPERTY(EditDefaultsOnly, Category = "UI|Pickup")
	TSubclassOf<class UQPPickupWidget> PickupWidgetClass; // 픽업 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Inventory")
	TSubclassOf<class UUserWidget> InventoryWidgetClass; // 인벤토리 위젯 클래스
	UPROPERTY()
	TObjectPtr<class UInventoryRootWidget> LootInventoryWidget; // 전리품 인벤토리 위젯 인스턴스

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UInventoryRootWidget> LootInventoryWidgetClass; // 전리품 인벤토리 위젯 클래스

	UPROPERTY(EditDefaultsOnly, Category = "UI|Pause")
	TSubclassOf<class UUserWidget> PauseMenuWidgetClass; // 일시정지 메뉴 위젯 클래스

private:
	//Pickup Widget Instance
	UPROPERTY()
	TObjectPtr<class UQPPickupWidget> PickupWidget; // 픽업 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidget; // 인벤토리 위젯 인스턴스

	UPROPERTY()
	TObjectPtr<class UUserWidget> PauseMenuWidget; // 일시정지 메뉴 인스턴스

	bool bPauseMenuOpen = false; // 일시정지 메뉴 열림 여부

	bool bInventoryOpen = false; // 인벤토리 열림 여부
	void SetInventoryOpen(bool bOpen); // 인벤토리 열림 상태 설정 함수
	bool bLootInventoryOpen = false; // 전리품 인벤토리 열림 여부
	bool bInventoryOpenedByLootKey = false; // 전리품 키로 인벤토리가 열렸는지 여부 초기화
	UPROPERTY(EditDefaultsOnly, Category = "UI|Loot")
	float LootScanRadius = 300.f; // 전리품 스캔 반경
	UPROPERTY()
	TObjectPtr<class ULootListWidget> CachedLootListWidget = nullptr; // 전리품 목록 위젯 인스턴스
	class UBorder* CachedLootPanelBorder = nullptr; // 전리품 목록 컨테이너 위젯 인스턴스
	class ULootListWidget* GetLootListWidget(); // 전리품 목록 위젯 가져오기 함수
	void SetLootListVisible(bool bVisible); // 전리품 목록 위젯 표시 상태 설정 함수
	bool IsLootListVisible() const; // 전리품 목록 위젯 표시 여부 확인 함수
	bool HasNearbyLoot(float Radius) const; // 근처에 전리품이 있는지 확인하는 함수
	void CloseLootInventoryWidget(bool bRestoreInventoryInputMode); // 전리품 인벤토리 위젯 닫기 함수
};
 