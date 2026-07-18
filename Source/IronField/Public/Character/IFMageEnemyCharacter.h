#pragma once

#include "CoreMinimal.h"
#include "Character/IFEnemyCharacter.h"
#include "IFMageEnemyCharacter.generated.h"

class UStaticMeshComponent;

UCLASS()
class IRONFIELD_API AIFMageEnemyCharacter : public AIFEnemyCharacter
{
	GENERATED_BODY()

public:
	AIFMageEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StaffMesh;

	void UpdateFocusOnTarget();
};
