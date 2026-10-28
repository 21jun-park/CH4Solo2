#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "CXMine.generated.h"

class USphereComponent;

UCLASS()
class CX_API ACXMine : public AActor
{
	GENERATED_BODY()

public:
	ACXMine();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USphereComponent> CollisionSphere;

	UFUNCTION()
	void OnMineOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UPROPERTY(EditDefaultsOnly, Category = "Mine")
	float DisableDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mine")
	float MineLifetime = 30.0f;

	virtual void BeginPlay() override;

private:
	bool bTriggered = false;

	void DestroyMine();

	FTimerHandle MineLifetimeTimerHandle;
};
