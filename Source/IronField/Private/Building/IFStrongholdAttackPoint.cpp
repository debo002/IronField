#include "Building/IFStrongholdAttackPoint.h"

AIFStrongholdAttackPoint::AIFStrongholdAttackPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scene component purely so the point has a placeable, visualizable transform in the Editor.
	USceneComponent* const DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
	RootComponent = DummyRoot;
}
