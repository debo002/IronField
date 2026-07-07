#include "Wave/IFEnemySpawnPoint.h"

AIFEnemySpawnPoint::AIFEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* const DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
	RootComponent = DummyRoot;
}
