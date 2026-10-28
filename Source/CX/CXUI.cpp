#include "CXUI.h"

// 속도를 KM/H 또는 MPH로 변환해 표시
void UCXUI::UpdateSpeed(float NewSpeed)
{
	// 차량 속도를 표시 단위에 맞게 변환
	const float FormattedSpeed =
		FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// 변환된 속도를 블루프린트에 전달
	OnSpeedUpdate(FormattedSpeed);
}

// 기어 표시 갱신
void UCXUI::UpdateGear(int32 NewGear)
{
	// 기어 정보를 블루프린트에 전달
	OnGearUpdate(NewGear);
}

// 코인 표시 갱신
void UCXUI::UpdateCoins(int32 NewCoins)
{
	// 코인 수를 블루프린트에 전달
	OnCoinsUpdate(NewCoins);
}

// 추가 속도 보너스 표시 갱신
void UCXUI::UpdateSpeedBonus(float NewBonus)
{
	OnSpeedBonusUpdate(NewBonus);
}

// 남은 시간 표시 갱신
void UCXUI::UpdateRemainingTime(float NewTime)
{
	OnRemainingTimeUpdate(NewTime);
}

// 승패 결과 표시 갱신
void UCXUI::UpdateResult(const FString& NewResult)
{
	OnResultUpdate(NewResult);
}

// 채팅 메시지를 블루프린트에 전달
void UCXUI::UpdateChatMessage(
	const FString& SenderName,
	const FString& Message)
{
	OnChatMessageUpdate(SenderName, Message);
}