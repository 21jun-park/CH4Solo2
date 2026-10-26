// CXGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

// 게임 진행 단계
UENUM(BlueprintType)
enum class ECXGamePhase : uint8
{
	Waiting,  // 플레이어 대기
	Playing,  // 게임 진행 중
	Finished  // 게임 종료
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// 현재 게임 진행 단계
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	ECXGamePhase GamePhase = ECXGamePhase::Waiting;

	// 게임에 남은 시간(초)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float RemainingTime = 0.0f;

	// 네트워크 복제 대상 등록
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};
