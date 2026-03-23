#include "QPPlayerController.h"
#include "PJ_Quiet_Protocol/UserWidget/QPPickupWidget.h"
#include "Blueprint/UserWidget.h"
#include "PJ_Quiet_Protocol/UserWidget/Inventory/InventoryRootWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "PJ_Quiet_Protocol/UserWidget/Inventory/LootListWidget.h"
#include "Components/PanelWidget.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "Components/Border.h"
#include "Components/Widget.h"

void AQPPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return; // 로컬 컨트롤러인지 확인
	if (PickupWidgetClass) {
		PickupWidget = CreateWidget<UQPPickupWidget>(this, PickupWidgetClass); // 위젯 인스턴스 생성
		if (PickupWidget) {
			PickupWidget->AddToViewport(999); // 뷰포트에 추가
			PickupWidget->SetTargetActor(nullptr); // 타겟 액터 초기화
			PickupWidget->SetVisibility(ESlateVisibility::Hidden); // 위젯 숨기기
		}
	}
	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass); // 인벤토리 위젯 인스턴스 생성
		if (InventoryWidget) {
			InventoryWidget->AddToViewport(10); // 뷰포트에 추가
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기

			SetLootListVisible(false); // 전리품 목록 위젯 숨기기
		}
	}
}

void AQPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AQPPlayerController::ToggleInventory);
	InputComponent->BindAction("ToggleLootInventory", IE_Pressed, this, &AQPPlayerController::ToggleLootInventory);
}

void AQPPlayerController::SetPickupTarget(AActor* NewTarget)
{
	if (!IsLocalController()) return; // 로컬 컨트롤러인지 확인
	if (!PickupWidget) return; // 픽업 위젯이 유효한지 확인
	if (NewTarget && !IsValid(NewTarget)) {
		NewTarget = nullptr; // 유효하지 않은 타겟은 nullptr로 설정
	}
	if (!PickupWidget->IsInViewport()) {
		PickupWidget->AddToViewport(999); // 뷰포트에 추가
	}
	PickupWidget->SetTargetActor(NewTarget); // 타겟 액터 설정
	const bool bShow = (NewTarget != nullptr);
	PickupWidget->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden); // 위젯 표시 여부 설정
}

void AQPPlayerController::ToggleInventory()
{
	SetLootListVisible(false); // 전리품 목록 위젯 숨기기
	bLootInventoryOpen = false; // 전리품 인벤토리 열림 상태 초기화

	const bool bNewOpen = !bInventoryOpen; // 새로운 인벤토리 열림 상태
	SetInventoryOpen(bNewOpen); // 인벤토리 열림 상태 설정
}

void AQPPlayerController::ToggleLootInventory()
{
	if (bInventoryOpen) // 닫기
	{
		bLootInventoryOpen = false; // 루팅 인벤토리 닫힘 상태 설정
		SetLootListVisible(false); // 루팅 목록 위젯 숨기기
		SetInventoryOpen(false); // 인벤토리 닫힘 상태 설정
		return;

	}
	if (!HasNearbyLoot(LootScanRadius)) return; // 근처에 전리품이 있는지 확인
	SetInventoryOpen(true); // 인벤토리 열림 상태 설정
	bLootInventoryOpen = true; // 루팅 인벤토리 열림 상태 설정
	SetLootListVisible(true); // 루팅 목록 위젯 표시
}

void AQPPlayerController::SetInventoryOpen(bool bOpen)
{
	if (!IsLocalPlayerController()) return; // 로컬 플레이어 컨트롤러인지 확인
	if (!bOpen) {
		SetLootListVisible(false); // 전리품 목록 위젯 숨기기
		bLootInventoryOpen = false; // 전리품 인벤토리 열림 상태
		bInventoryOpenedByLootKey = false; // 전리품 키로 인벤토리가 열렸는지 여부 초기화
		CloseLootInventoryWidget(false); // 전리품 인벤토리 위젯 닫기
	}
	bInventoryOpen = bOpen; // 인벤토리 열림 상태 설정

	if (!InventoryWidget && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass); // 인벤토리 위젯 인스턴스 생성
		if (InventoryWidget) {
			InventoryWidget->AddToViewport(10); // 뷰포트에 추가
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기
		}
	}

	if (!InventoryWidget) return; // 인벤토리 위젯이 유효한지 확인

	if (bInventoryOpen) {
		InventoryWidget->SetVisibility(ESlateVisibility::Visible); // 인벤토리 위젯 표시
		SetShowMouseCursor(true); // 마우스 커서 표시
		bEnableClickEvents = true; // 클릭 이벤트 활성화

		//이동/시야 입력 차단
		SetIgnoreMoveInput(true); // 이동 입력 무시
		SetIgnoreLookInput(true); // 시야 입력 무시

		FInputModeGameAndUI InputMode; // 입력 모드 설정
		InputMode.SetHideCursorDuringCapture(false); // 커서 캡처 중 커서 숨기지 않음
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스 잠금 동작 설정
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget()); // 위젯에 포커스 설정
		SetInputMode(InputMode); // 입력 모드 적용
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기
		SetShowMouseCursor(false); // 마우스 커서 숨기기
		bEnableClickEvents = false; // 클릭 이벤트 비활성화
		bEnableMouseOverEvents = false; // 마우스 오버 이벤트 비활성화
		//이동/시야 입력 허용
		SetIgnoreMoveInput(false); // 이동 입력 허용
		SetIgnoreLookInput(false); // 시야 입력 허용
		FInputModeGameOnly InputMode; // 게임 전용 입력 모드 설정
		SetInputMode(InputMode); // 입력 모드 적용
	}
}

static ULootListWidget* FindLootListRecursive(UWidget* Widget) // 재귀적으로 ULootListWidget 찾기
{
	if (!Widget) return nullptr; // 유효성 검사

	if (ULootListWidget* Loot = Cast<ULootListWidget>(Widget)) // ULootListWidget인지 확인
	{
		return Loot; // 찾았으면 반환
	}

	// UserWidget 안에 또 UserWidget이 들어있는 구조도 대비
	if (UUserWidget* AsUserWidget = Cast<UUserWidget>(Widget))
	{
		return FindLootListRecursive(AsUserWidget->GetRootWidget()); // 루트 위젯에서 재귀적으로 찾기
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget)) // 패널 위젯인지 확인
	{
		const int32 ChildCount = Panel->GetChildrenCount(); // 자식 위젯 개수 가져오기
		for (int32 i = 0; i < ChildCount; ++i) // 자식 위젯 순회
		{
			if (UWidget* Child = Panel->GetChildAt(i)) // 자식 위젯 가져오기
			{
				if (ULootListWidget* Found = FindLootListRecursive(Child)) // 재귀적으로 찾기
				{
					return Found;
				}
			}
		}
	}

	return nullptr; // 못 찾았으면 nullptr 반환
}

ULootListWidget* AQPPlayerController::GetLootListWidget()
{
	if (CachedLootListWidget && IsValid(CachedLootListWidget))
	{
		return CachedLootListWidget; // 캐시된 위젯이 유효하면 반환
	}
	if (!InventoryWidget) return nullptr; // 인벤토리 위젯이 유효한지 확인

	UWidget* Root = InventoryWidget->GetRootWidget(); // 인벤토리 위젯의 루트 위젯 가져오기
	CachedLootListWidget = FindLootListRecursive(Root); // 재귀적으로 ULootListWidget 찾기
	CachedLootPanelBorder = nullptr; // 캐시된 패널 경계 초기화

	if (CachedLootListWidget) // 찾았으면
	{
		UWidget* Current = CachedLootListWidget; // 현재 위젯 설정
		for (int32 Guard = 0; Guard < 32 && Current; ++Guard) // 최대 32단계까지 부모 위젯 순회
		{
			UPanelWidget* ParentPanel = Current->GetParent(); // 부모 패널 위젯 가져오기
			if (!ParentPanel) break;
			if (UBorder* Border = Cast<UBorder>(ParentPanel)) // UBorder인지 확인
			{
				CachedLootPanelBorder = Border; // 캐시된 패널 경계 설정
				break;
			}
			Current = Cast<UWidget>(ParentPanel); // 부모 위젯으로 이동
			if (!Current) break;
		}
	}

	return CachedLootListWidget; // 캐시된 ULootListWidget 반환
}

void AQPPlayerController::SetLootListVisible(bool bVisible)
{
	if (ULootListWidget* Loot = GetLootListWidget()) // ULootListWidget 가져오기
	{
		Loot->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); // 위젯 가시성 설정
	}
	if (CachedLootPanelBorder)
	{
		UWidget* PanelWidget = Cast<UWidget>(CachedLootPanelBorder); // 패널 위젯 설정
		PanelWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); // 패널 위젯 가시성 설정
	}
}

bool AQPPlayerController::IsLootListVisible() const
{
	if (CachedLootListWidget && IsValid(CachedLootListWidget)) // 캐시된 위젯이 유효한지 확인
	{
		const ESlateVisibility Vis = CachedLootListWidget->GetVisibility(); // 위젯의 가시성 가져오기
		return (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden); // 표시 상태인지 확인
	}
	return false;
}

bool AQPPlayerController::HasNearbyLoot(float Radius) const
{
	const APawn* pawn = GetPawn();
	if (!pawn) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector Center = pawn->GetActorLocation(); // 중심 위치 설정

	FCollisionObjectQueryParams ObjParams; // 충돌 객체 쿼리 파라미터 설정
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 동적 월드 객체 포함
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic); // 정적 월드 객체 포함
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody); // 물리 바디 객체 포함

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LootKeyScan), false, pawn); // 충돌 쿼리 파라미터 설정
	Params.AddIgnoredActor(pawn); // 플레이어 자신 무시

	TArray<FOverlapResult> Overlaps; // 오버랩 결과 배열
	const bool bOverlapped = World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjParams, FCollisionShape::MakeSphere(Radius), Params); // 오버랩 검사 수행

	if (!bOverlapped) return false;

	for (const FOverlapResult& Results : Overlaps) // 오버랩 결과 순회
	{
		AActor* actors = Results.GetActor(); // 액터 가져오기
		if (!actors) continue; //	유효성 검사

		if (AWeaponBase* Weapon = Cast<AWeaponBase>(actors)) // 무기 액터인지 확인
		{
			if (Weapon->GetOwner() == nullptr && Weapon->GetWeaponItemData() != nullptr) // 소유자가 없고 아이템 데이터가 있는지 확인
			{
				return true; // 근처에 전리품이 있음
			}
		}

		if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(actors)) // 월드 아이템 액터인지 확인
		{
			if (WorldItem->ItemData && WorldItem->Quantity > 0) // 아이템 데이터가 있고 수량이 0보다 큰지 확인
			{
				return true; // 근처에 전리품이 있음
			}
		}
	}

	return false;	// 근처에 전리품이 없음
}

void AQPPlayerController::CloseLootInventoryWidget(bool bRestoreInventoryInputMode)
{
	if (!LootInventoryWidget) return; // 전리품 인벤토리 위젯이 유효한지 확인

	LootInventoryWidget->RemoveFromParent(); // 전리품 인벤토리 위젯 제거

	//인벤이 열려있다면 인벤 input 모드를 다시 확실히 적용
	if (bRestoreInventoryInputMode && bInventoryOpen)
	{
		SetInventoryOpen(true); // 인벤토리 열림 상태 설정
	}
	else if (!bInventoryOpen)
	{
		SetInputMode(FInputModeGameOnly()); // 게임 전용 입력 모드 설정
		bShowMouseCursor = false; // 마우스 커서 숨기기
	}
}
