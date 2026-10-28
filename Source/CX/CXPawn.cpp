
#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "CXMine.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	bReplicates = true;
	SetReplicateMovement(true);
	// 전방 카메라 설정
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// 후방 카메라 설정
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// 차량 메시 설정
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// 차량 이동 컴포넌트 가져오기
	ChaosVehicleMovement =
		CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
}

// 시작 시 기본 엔진 토크 저장
void ACXPawn::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(ChaosVehicleMovement))
	{
		BaseEngineTorque = ChaosVehicleMovement->EngineSetup.MaxTorque;
	}
}

void ACXPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 조향
		EnhancedInputComponent->BindAction(
			SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(
			SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// 가속
		EnhancedInputComponent->BindAction(
			ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(
			ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// 브레이크
		EnhancedInputComponent->BindAction(
			BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(
			BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(
			BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// 핸드브레이크
		EnhancedInputComponent->BindAction(
			HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(
			HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// 주변 둘러보기
		EnhancedInputComponent->BindAction(
			LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// 카메라 전환
		EnhancedInputComponent->BindAction(
			ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// 공격
		EnhancedInputComponent->BindAction(
			AttackAction, ETriggerEvent::Started, this, &ACXPawn::Attack);

		// 지뢰 설치
		EnhancedInputComponent->BindAction(
			MineAction, ETriggerEvent::Started, this, &ACXPawn::PlaceMine);

		// 차량 리셋
		EnhancedInputComponent->BindAction(
			ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(
			LogTemplateVehicle,
			Error,
			TEXT("'%s' Failed to find an Enhanced Input component!"),
			*GetNameSafe(this)
		);
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// 공중에서 차량의 회전 감쇠 적용
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// 카메라의 좌우 회전을 정면으로 복귀
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);
	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	// 속도 아이템 보너스를 엔진 토크에 적용
	AController* VehicleController = GetController();

	if (IsValid(VehicleController) && IsValid(ChaosVehicleMovement))
	{
		ACXPlayerState* MyPlayerState =
			VehicleController->GetPlayerState<ACXPlayerState>();

		if (IsValid(MyPlayerState) &&
			!FMath::IsNearlyEqual(AppliedSpeedBonus, MyPlayerState->BonusMaxSpeed))
		{
			AppliedSpeedBonus = MyPlayerState->BonusMaxSpeed;

			const float TorqueMultiplier =
				1.0f + (AppliedSpeedBonus / 100.0f);

			ChaosVehicleMovement->SetMaxEngineTorque(
				BaseEngineTorque * TorqueMultiplier);
		}
	}
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		ChaosVehicleMovement->SetSteeringInput(0.0f);
		return;
	}

	const float SteeringValue = Value.Get<float>();
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// 게임 진행 중이 아니거나 지뢰 효과를 받으면 가속 차단
	if (!IsGamePlaying() || bMineDisabled)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		return;
	}

	const float ThrottleValue = Value.Get<float>();
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		ChaosVehicleMovement->SetBrakeInput(0.0f);
		return;
	}

	const float BreakValue = Value.Get<float>();
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}

	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	BrakeLights(false);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}

	ChaosVehicleMovement->SetHandbrakeInput(true);
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	const float LookValue = Value.Get<float>();
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}

	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	SetActorTransform(
		FTransform(ResetRotation, ResetLocation, FVector::OneVector),
		false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

void ACXPawn::Attack(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}

	UE_LOG(LogTemplateVehicle, Warning, TEXT("Attack 입력 감지"));
	ServerAttack();
}

// 서버에서 공격 처리
void ACXPawn::ServerAttack_Implementation()
{
	if (!IsGamePlaying())
	{
		return;
	}

	// 서버에서 공격 간격 검사
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return;
	}

	LastAttackTime = CurrentTime;

	UE_LOG(LogTemplateVehicle, Warning, TEXT("ServerAttack 실행됨"));

	// 차량 앞쪽에서 탐지 시작
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 200.0f;
	const FVector End = Start + GetActorForwardVector() * 1000.0f;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Vehicle,
		QueryParams
	);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		bHit ? FColor::Green : FColor::Red,
		false,
		2.0f,
		0,
		3.0f
	);

	if (bHit)
	{
		ACXPawn* HitPawn = Cast<ACXPawn>(HitResult.GetActor());

		if (IsValid(HitPawn) && HitPawn != this)
		{
			ACXPlayerState* AttackerState =
				GetController()
				? GetController()->GetPlayerState<ACXPlayerState>()
				: nullptr;

			AController* HitController = HitPawn->GetController();

			ACXPlayerState* TargetState =
				HitController
				? HitController->GetPlayerState<ACXPlayerState>()
				: nullptr;

			if (IsValid(AttackerState) && IsValid(TargetState))
			{
				const int32 StealAmount = FMath::Min(TargetState->Coins, 5);

				if (StealAmount > 0)
				{
					TargetState->AddCoins(-StealAmount);
					AttackerState->AddCoins(StealAmount);
				}
			}
		}
	}
}

// F키 입력 시 지뢰 설치 요청
void ACXPawn::PlaceMine(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}

	ServerPlaceMine();
}

// 서버에서 차량 뒤에 지뢰 생성
void ACXPawn::ServerPlaceMine_Implementation()
{
	if (!IsGamePlaying() || !IsValid(MineClass))
	{
		return;
	}

	const FVector SpawnLocation =
		GetActorLocation() - GetActorForwardVector() * 250.0f
		+ FVector(0.0f, 0.0f, 20.0f);

	const FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	GetWorld()->SpawnActor<ACXMine>(
		MineClass,
		SpawnTransform,
		SpawnParams
	);
}

// 지뢰에 맞았을 때 가속 차단
void ACXPawn::ApplyMineEffect(float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	bMineDisabled = true;

	// 기존 효과가 남아 있다면 타이머를 갱신
	GetWorldTimerManager().ClearTimer(MineEffectTimerHandle);
	GetWorldTimerManager().SetTimer(
		MineEffectTimerHandle,
		this,
		&ACXPawn::ResetMineEffect,
		Duration,
		false
	);
}

// 지뢰 효과 종료
void ACXPawn::ResetMineEffect()
{
	bMineDisabled = false;
}

// 현재 게임이 진행 중인지 확인
bool ACXPawn::IsGamePlaying() const
{
	const ACXGameState* CXState =
		GetWorld() ? GetWorld()->GetGameState<ACXGameState>() : nullptr;

	return CXState &&
		CXState->GamePhase == ECXGamePhase::Playing;
}

// 지뢰 효과 변수 복제 등록
void ACXPawn::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACXPawn, bMineDisabled);
}

#undef LOCTEXT_NAMESPACE
