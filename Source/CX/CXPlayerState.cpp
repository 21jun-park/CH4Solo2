// CXPlayerState.cpp


#include "CXPlayerState.h"
#include "Net/UnrealNetwork.h"

// 코인 변경: 서버에서만 실행
void ACXPlayerState::AddCoins(int32 Amount)
{
	if (!HasAuthority())
	{
		return; // 서버가 아니면 변경하지 않음
	}

	// 코인이 0 미만으로 내려가지 않도록 제한
	Coins = FMath::Max(0, Coins + Amount);
}

// 추가 이동 속도 변경: 서버에서만 실행
void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (!HasAuthority())
	{
		return; // 서버가 아니면 변경하지 않음
	}

	// 속도 보너스가 음수가 되지 않도록 제한
	BonusMaxSpeed = FMath::Max(0.0f, BonusMaxSpeed + Amount);
}

// 플레이어 데이터의 네트워크 복제 설정
void ACXPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 코인과 속도 보너스는 해당 플레이어 본인에게만 전달
	DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}