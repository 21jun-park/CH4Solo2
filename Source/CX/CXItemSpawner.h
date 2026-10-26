#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

protected:
	virtual void BeginPlay() override;

	// 생성할 아이템 종류를 블루프린트에서 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSubclassOf<class ACXItem> ItemClass;

	// 아이템 재생성까지 대기할 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float RespawnDelay = 2.0f;

	// 아이템 생성
	void SpawnItem();

	// 생성된 아이템이 파괴되면 호출
	UFUNCTION()
	void OnSpawnedItemDestroyed(AActor* DestroyedActor);

	// 재생성 타이머
	FTimerHandle RespawnTimerHandle;
};



