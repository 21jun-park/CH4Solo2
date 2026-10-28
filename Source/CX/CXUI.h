#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXUI.generated.h"

UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 속도를 MPH 또는 KM/H로 표시할지 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

public:
	// 속도 표시 갱신
	void UpdateSpeed(float NewSpeed);

	// 기어 표시 갱신
	void UpdateGear(int32 NewGear);

	// 코인 표시 갱신
	void UpdateCoins(int32 NewCoins);

	// 추가 속도 보너스 표시 갱신
	void UpdateSpeedBonus(float NewBonus);

	// 남은 시간 표시 갱신
	void UpdateRemainingTime(float NewTime);

	// 승패 결과 표시 갱신
	void UpdateResult(const FString& NewResult);

	// 블루프린트에서 채팅 메시지 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Chat)
	void OnChatMessageUpdate(const FString& SenderName, const FString& Message);

	// 채팅 메시지 표시 갱신
	void UpdateChatMessage(const FString& SenderName, const FString& Message);

protected:
	// 블루프린트에서 추가 속도 보너스 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedBonusUpdate(float NewBonus);

	// 블루프린트에서 남은 시간 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnRemainingTimeUpdate(float NewTime);

	// 블루프린트에서 승패 결과 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnResultUpdate(const FString& NewResult);

protected:
	// 블루프린트에서 속도 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	// 블루프린트에서 기어 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);

	// 블루프린트에서 코인 표시 구현
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnCoinsUpdate(int32 NewCoins);
};