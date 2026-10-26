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