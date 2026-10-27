// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();
	// 게임 시작 및 제한 시간 관리
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 게임 시작
	void StartGame();

	// 게임 종료 및 승자 판정
	void FinishGame();

private:
	float GameDuration = 180.0f; // 제한 시간 3분
	bool bGameStarted = false;

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

private:
	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

};



