// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "CXPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CXGameState.h"
#include "Net/UnrealNetwork.h"

// 게임 시작 시 UI 생성
void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어만 UI를 생성
	if (IsLocalController() == false)
	{
		return;
	}

	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);
	check(VehicleUI);

	VehicleUI->AddToViewport();
}

// 입력 설정
void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 기본 입력 매핑 등록
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// 조향 장치 사용 시 추가 입력 매핑 등록
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

// 네트워크 복제 대상 등록
void ACXPlayerController::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

// 매 프레임 UI 갱신
void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		// 현재 차량 속도 표시
		VehicleUI->UpdateSpeed(
			VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed()
		);

		// 현재 기어 표시
		VehicleUI->UpdateGear(
			VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear()
		);

		// 내 PlayerState의 코인 표시
		if (ACXPlayerState* MyPlayerState =
			GetPlayerState<ACXPlayerState>())
		{
			VehicleUI->UpdateCoins(MyPlayerState->Coins);

			// 추가 속도 보너스 표시
			VehicleUI->UpdateSpeedBonus(MyPlayerState->BonusMaxSpeed);

		}

		// 남은 시간 표시
		if (ACXGameState* MyGameState =
			GetWorld()->GetGameState<ACXGameState>())
		{
			VehicleUI->UpdateRemainingTime(MyGameState->RemainingTime);
		}
	}
}

// 승패 결과를 UI에 표시
void ACXPlayerController::UpdateResultUI_Implementation(const FString& Result)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("UpdateResultUI: IsValid(VehicleUI) = %d"),
		IsValid(VehicleUI)
	);

	if (IsValid(VehicleUI))
	{
		VehicleUI->UpdateResult(Result);
	}
}

// 차량 소유 시 차량 참조 저장
void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}