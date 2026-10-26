// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "CXPlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
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

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 조향
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// 가속
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// 브레이크
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// 핸드브레이크
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// 주변 둘러보기
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// 카메라 전환
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// 공격
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Started,
			this,
			&ACXPawn::Attack
		);

		// 차량 리셋
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
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
	float SteeringValue = Value.Get<float>();
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	float ThrottleValue = Value.Get<float>();
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	float BreakValue = Value.Get<float>();
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	BrakeLights(false);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
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
	float LookValue = Value.Get<float>();
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
	UE_LOG(LogTemplateVehicle, Warning, TEXT("Attack 입력 감지"));
	ServerAttack();
}

// 서버에서 공격 처리
void ACXPawn::ServerAttack_Implementation()
{
	UE_LOG(LogTemplateVehicle, Warning, TEXT("ServerAttack 실행됨"));

	// 차량 앞쪽에서 탐지 시작
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 200.0f;
	const FVector End = Start + GetActorForwardVector() * 1000.0f;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// 전방으로 선을 쏴서 차량 탐지
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Vehicle,
		QueryParams
	);

	// 탐지 결과를 디버그 선으로 표시
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
		// 맞은 대상이 차량인지 확인
		ACXPawn* HitPawn = Cast<ACXPawn>(HitResult.GetActor());

		// 상대 차량이고, 자기 차량이 아닐 때만 처리
		if (IsValid(HitPawn) && HitPawn != this)
		{
			// 공격자 PlayerState 가져오기
			ACXPlayerState* AttackerState =
				GetController()
				? GetController()->GetPlayerState<ACXPlayerState>()
				: nullptr;

			// 상대 차량의 PlayerState 가져오기
			AController* HitController = HitPawn->GetController();

			ACXPlayerState* TargetState =
				HitController
				? HitController->GetPlayerState<ACXPlayerState>()
				: nullptr;

			// 양쪽 PlayerState 확인
			if (IsValid(AttackerState) && IsValid(TargetState))
			{
				UE_LOG(
					LogTemplateVehicle,
					Warning,
					TEXT("PlayerState"));
				
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

