
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItem.generated.h"

// 아이템 종류
UENUM(BlueprintType)
enum class ECXItemType : uint8
{
	Coin,   // 코인 획득
	Speed   // 속도 보너스 획득
};

UCLASS()
class CX_API ACXItem : public AActor
{
	GENERATED_BODY()

public:
	// 기본 설정
	ACXItem();

protected:
	// 아이템 충돌 감지 영역
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<class USphereComponent> CollisionSphere;

	// 아이템 외형
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<class UStaticMeshComponent> ItemMesh;

	// 아이템 종류: 블루프린트에서 선택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ECXItemType ItemType = ECXItemType::Coin;

	// 아이템 획득 시 호출
	UFUNCTION()
	void OnItemOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

protected:
	virtual void BeginPlay() override;
};