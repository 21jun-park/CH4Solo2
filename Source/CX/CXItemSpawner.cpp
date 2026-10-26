#include "CXItemSpawner.h"
#include "CXItem.h"
#include "Engine/World.h"
#include "TimerManager.h"

// 기본 설정
ACXItemSpawner::ACXItemSpawner()
{
	// 매 프레임 실행할 작업이 없으므로 Tick 비활성화
	PrimaryActorTick.bCanEverTick = false;
}

// 게임 시작 시 아이템 생성
void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 아이템 생성은 서버에서만 처리
	if (HasAuthority())
	{
		SpawnItem();
	}
}

// 아이템 생성
void ACXItemSpawner::SpawnItem()
{
	if (!HasAuthority() || !IsValid(ItemClass))
	{
		return;
	}

	// 스포너 위치에 아이템 생성
	ACXItem* SpawnedItem = GetWorld()->SpawnActor<ACXItem>(
		ItemClass,
		GetActorTransform()
	);

	if (IsValid(SpawnedItem))
	{
		// 아이템이 파괴되면 재생성 예약
		SpawnedItem->OnDestroyed.AddDynamic(
			this,
			&ACXItemSpawner::OnSpawnedItemDestroyed
		);
	}
}

// 아이템 파괴 시 호출
void ACXItemSpawner::OnSpawnedItemDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// 2초 후 아이템 재생성
	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&ACXItemSpawner::SpawnItem,
		RespawnDelay,
		false
	);
}