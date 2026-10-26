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