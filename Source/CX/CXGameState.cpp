// CXGameState.cpp


#include "CXGameState.h"
#include "Net/UnrealNetwork.h"

// 게임 상태와 남은 시간을 클라이언트에 복제
void ACXGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 모든 플레이어가 게임 진행 단계를 알 수 있도록 복제
	DOREPLIFETIME(ACXGameState, GamePhase);

	// 모든 플레이어가 남은 시간을 알 수 있도록 복제
	DOREPLIFETIME(ACXGameState, RemainingTime);
}
