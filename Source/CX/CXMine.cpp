
#include "CXMine.h"

#include "CXPawn.h"
#include "Components/SphereComponent.h"
#include "TimerManager.h"

ACXMine::ACXMine()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(
		TEXT("CollisionSphere"));

	SetRootComponent(CollisionSphere);

	CollisionSphere->InitSphereRadius(100.0f);
	CollisionSphere->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(
		ECC_Vehicle, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
}

void ACXMine::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(
			this, &ACXMine::OnMineOverlap);

		// 일정 시간이 지나면 지뢰 제거
		GetWorldTimerManager().SetTimer(
			MineLifetimeTimerHandle,
			this,
			&ACXMine::DestroyMine,
			MineLifetime,
			false
		);
	}
}

void ACXMine::OnMineOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || bTriggered || !IsValid(OtherActor))
	{
		return;
	}

	// 지뢰를 설치한 차량은 무시
	if (OtherActor == GetOwner())
	{
		return;
	}

	ACXPawn* HitVehicle = Cast<ACXPawn>(OtherActor);
	if (!IsValid(HitVehicle))
	{
		return;
	}

	bTriggered = true;

	CollisionSphere->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	// 피격 차량의 가속을 일정 시간 차단
	HitVehicle->ApplyMineEffect(DisableDuration);

	Destroy();
}

void ACXMine::DestroyMine()
{
	if (HasAuthority())
	{
		Destroy();
	}
}
