#include "LootListWidget.h"
#include "Components/ScrollBox.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "LootListEntryWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "Engine/OverlapResult.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
void ULootListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RefreshTimer,
			this,
			&ULootListWidget::RefreshLootList,
			RefreshInterval,
			true
		);
	}
}

void ULootListWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
	}
	Super::NativeDestruct();
}

void ULootListWidget::RefreshLootList()
{
	const ESlateVisibility WidgetVisibility = GetVisibility();
	if(WidgetVisibility == ESlateVisibility::Collapsed || WidgetVisibility == ESlateVisibility::Hidden)
	{
		return;
	}
	if (!LootScroll)
	{
		return;
	}
	if (!EntryWidgetClass)
	{
		return;
	}

	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn());
	if (!Character)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Center = Character->GetActorLocation();

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(ScanRadius);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LootScan), false, Character);
	Params.AddIgnoredActor(Character);

	TArray<FOverlapResult> Overlaps;
	const bool bOverlapped = World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjParams, Sphere, Params);

	struct FLootRow
	{
		TWeakObjectPtr<AActor> Actor;
		TObjectPtr<UItemDataAsset> ItemData = nullptr;
		int32 Quantity = 1;
		float DistSq = 0.f;
	};

	TArray<FLootRow> Rows;
	Rows.Reserve(Overlaps.Num());

	TSet<TWeakObjectPtr<AActor>> UniqueActors;

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* A = R.GetActor();
		if (!A || UniqueActors.Contains(A)) continue;
		UniqueActors.Add(A);

		if (AWeaponBase* Weapon = Cast<AWeaponBase>(A))
		{
			if (Weapon->GetOwner() != nullptr) continue;

			UItemDataAsset* WeaponItemData = Weapon->GetWeaponItemData();
			if (!WeaponItemData) continue;

			FLootRow Row;
			Row.Actor = Weapon;
			Row.ItemData = WeaponItemData;
			Row.Quantity = 1;
			Row.DistSq = FVector::DistSquared(Center, Weapon->GetActorLocation());
			Rows.Add(Row);
			continue;
		}

		if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(A))
		{
			if (!WorldItem->ItemData || WorldItem->Quantity <= 0) continue;

			FLootRow Row;
			Row.Actor = WorldItem;
			Row.ItemData = WorldItem->ItemData;
			Row.Quantity = WorldItem->Quantity;
			Row.DistSq = FVector::DistSquared(Center, WorldItem->GetActorLocation());
			Rows.Add(Row);
		}
	}
	Rows.Sort([](const FLootRow& L, const FLootRow& R) { return L.DistSq < R.DistSq; });

	LootScroll->ClearChildren();
	for (const FLootRow& Row : Rows)
	{
		if (!Row.Actor.IsValid() || !Row.ItemData) continue;

		ULootListEntryWidget* Entry = CreateWidget<ULootListEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
		if (!Entry) continue;

		Entry->Setup(Row.Actor.Get(), Row.ItemData, Row.Quantity);
		LootScroll->AddChild(Entry);
	}
}
