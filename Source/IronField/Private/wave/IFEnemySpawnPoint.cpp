#include "Wave/IFEnemySpawnPoint.h"

#include "Components/BillboardComponent.h"

AIFEnemySpawnPoint::AIFEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* const DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
	RootComponent = DummyRoot;

#if WITH_EDITORONLY_DATA
	if (UBillboardComponent* const SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif
}
