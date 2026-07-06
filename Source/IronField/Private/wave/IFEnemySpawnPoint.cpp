#include "Wave/IFEnemySpawnPoint.h"

AIFEnemySpawnPoint::AIFEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// In the Editor, a scene component is useful to visualizes the spawn point's transform.
	USceneComponent* const DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
	RootComponent = DummyRoot;
}
