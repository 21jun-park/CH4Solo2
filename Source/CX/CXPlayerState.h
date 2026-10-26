// CXPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 현재 보유 코인
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 Coins = 0;

	// 추가 이동 속도
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float BonusMaxSpeed = 0.0f;

	// 코인 변경
	void AddCoins(int32 Amount);

	// 속도 보너스 변경
	void AddBonusMaxSpeed(float Amount);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};
