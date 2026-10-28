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

// 서버에서 채팅 메시지를 받아 모든 플레이어에게 전달
void ACXPlayerController::ServerSendChatMessage_Implementation(
	const FString& Message)
{
	// 빈 메시지는 전송하지 않음
	if (Message.TrimStartAndEnd().IsEmpty())
	{
		return;
	}

	// 보낸 플레이어 이름 가져오기
	const FString SenderName = PlayerState
		? PlayerState->GetPlayerName()
		: TEXT("Player");

	// 모든 플레이어 컨트롤러에 메시지 전달
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get()))
		{
			PC->ClientReceiveChatMessage(SenderName, Message);
		}
	}
}

// 클라이언트에서 채팅 메시지를 받음
void ACXPlayerController::ClientReceiveChatMessage_Implementation(
	const FString& SenderName,
	const FString& Message)
{
	// 받은 채팅을 현재 플레이어의 UI에 표시
	if (IsValid(VehicleUI))
	{
		VehicleUI->UpdateChatMessage(SenderName, Message);
	}
}

// 블루프린트의 채팅 전송 요청을 서버 RPC로 전달
void ACXPlayerController::SendChatMessage(const FString& Message)
{
	ServerSendChatMessage(Message);
}