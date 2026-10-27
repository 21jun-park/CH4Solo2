// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

// 게임 모드 초기 설정
ACXGameMode::ACXGameMode()
{
	// 플레이어 컨트롤러 클래스 지정
	PlayerControllerClass = ACXPlayerController::StaticClass();

	// 개인 데이터 관리 클래스 지정
	PlayerStateClass = ACXPlayerState::StaticClass();

	// 공용 게임 상태 관리 클래스 지정
	GameStateClass = ACXGameState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
}

// 차량이 겹치지 않는 플레이어 시작 위치 선택
AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(
		AvailablePlayerStarts.IsEmpty() == false,
		TEXT("There is no available Vehicle PlayerStart.")
	);

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());

	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

// 시작 위치 주변에 차량이 있는지 확인
bool ACXGameMode::IsPlayerStartAvailable(
	const APlayerStart* PlayerStart
) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform =
		PlayerStart->GetActorTransform();

	const FVector CheckLocation =
		PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);

	const FQuat CheckRotation =
		PlayerStartTransform.GetRotation();

	const FCollisionShape CheckShape =
		FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VehiclePlayerStartCheck),
		false
	);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}

// 게임 시작 시 초기화
void ACXGameMode::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay"));

	Super::BeginPlay();

	// 서버에서만 게임 시작
	if (HasAuthority())
	{
		StartGame();
	}
}

// 게임 진행
void ACXGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bGameStarted)
	{
		return;
	}

	ACXGameState* CXState = GetGameState<ACXGameState>();
	if (!IsValid(CXState))
	{
		return;
	}

	CXState->RemainingTime -= DeltaSeconds;

	static int32 LastDisplayedSecond = -1;
	const int32 CurrentSecond = FMath::CeilToInt(CXState->RemainingTime);

	if (CurrentSecond != LastDisplayedSecond)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Remaining Time: %d"),
			CurrentSecond
		);

		LastDisplayedSecond = CurrentSecond;
	}

	if (CXState->RemainingTime <= 0.0f)
	{
		CXState->RemainingTime = 0.0f;
		FinishGame();
	}
}

// 게임 시작
void ACXGameMode::StartGame()
{
	ACXGameState* CXState = GetGameState<ACXGameState>();
	if (!IsValid(CXState))
	{
		return;
	}

	CXState->GamePhase = ECXGamePhase::Playing;
	CXState->RemainingTime = GameDuration;
	bGameStarted = true;
}

// 게임 종료 및 승자 판정
void ACXGameMode::FinishGame()
{
	if (!bGameStarted)
	{
		return;
	}

	bGameStarted = false;

	ACXGameState* CXState = GetGameState<ACXGameState>();
	if (!IsValid(CXState))
	{
		return;
	}

	CXState->GamePhase = ECXGamePhase::Finished;

	// 게임 종료 시 모든 플레이어 차량 입력 차단
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->SetIgnoreMoveInput(true);
		}
	}

	// 코인이 가장 많은 플레이어 판정
	ACXPlayerState* Winner = nullptr;
	bool bIsDraw = false;

	for (APlayerState* PlayerState : CXState->PlayerArray)
	{
		ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState);
		if (!IsValid(CXPlayerState))
		{
			continue;
		}

		if (!Winner || CXPlayerState->Coins > Winner->Coins)
		{
			Winner = CXPlayerState;
			bIsDraw = false;
		}
		else if (CXPlayerState->Coins == Winner->Coins)
		{
			bIsDraw = true;
		}
	}

	// 각 플레이어의 화면에 결과 표시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get());
		if (!IsValid(PC))
		{
			continue;
		}

		ACXPlayerState* PlayerState = PC->GetPlayerState<ACXPlayerState>();
		if (!IsValid(PlayerState))
		{
			continue;
		}

		FString Result;

		if (bIsDraw)
		{
			Result = TEXT("Draw!");
		}
		else if (PlayerState == Winner)
		{
			Result = TEXT("Win!");
		}
		else
		{
			Result = TEXT("Lose!");
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Result UI call: %s -> %s"),
			*PlayerState->GetPlayerName(),
			*Result
		);

		PC->UpdateResultUI(Result);
	}

	// 승자 로그
	if (Winner)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Winner: %s | Coins: %d"),
			*Winner->GetPlayerName(),
			Winner->Coins
		);
	}
}