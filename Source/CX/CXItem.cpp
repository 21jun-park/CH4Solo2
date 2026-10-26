
#include "CXItem.h"
#include "CXPlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

// 아이템 기본 설정
ACXItem::ACXItem()
{
	// 아이템을 네트워크에 복제
	bReplicates = true;

	// 충돌 감지 영역 생성
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);

	CollisionSphere->SetSphereRadius(150.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	// 아이템 외형 생성
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(CollisionSphere);

	// 외형 자체는 충돌하지 않도록 설정
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 매 프레임 실행할 작업이 없으므로 Tick 비활성화
	PrimaryActorTick.bCanEverTick = false;
}

// 게임 시작 시 충돌 이벤트 연결
void ACXItem::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&ACXItem::OnItemOverlap
	);
}

// 차량이 아이템을 획득했을 때 호출
void ACXItem::OnItemOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	// 보상 지급은 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	// 충돌한 대상이 유효한 플레이어 차량인지 확인
	APawn* Vehicle = Cast<APawn>(OtherActor);
	if (!IsValid(Vehicle))
	{
		return;
	}

	// 차량을 조종하는 플레이어의 상태 가져오기
	AController* VehicleController = Vehicle->GetController();
	if (!IsValid(VehicleController))
	{
		return;
	}

	ACXPlayerState* PlayerState =
		VehicleController->GetPlayerState<ACXPlayerState>();

	if (!IsValid(PlayerState))
	{
		return;
	}

	// 아이템 종류에 따라 보상 지급
	switch (ItemType)
	{
	case ECXItemType::Coin:
		// 코인 아이템: 코인 10개 지급
		PlayerState->AddCoins(10);
		break;

	case ECXItemType::Speed:
		// 속도 아이템: 속도 보너스 10 증가
		PlayerState->AddBonusMaxSpeed(10.0f);
		break;
	}

	// 획득한 아이템 제거
	Destroy();
}
