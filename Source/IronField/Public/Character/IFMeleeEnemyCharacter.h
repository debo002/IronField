#pragma once

#include "CoreMinimal.h"
#include "Character/IFEnemyCharacter.h"
#include "Combat/IFWeaponBoxOwner.h"
#include "IFMeleeEnemyCharacter.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Close-range enemy. Installs UIFMeleeCombatComponent (weapon-box hits, reactive block). */
UCLASS()
class IRONFIELD_API AIFMeleeEnemyCharacter : public AIFEnemyCharacter, public IIFWeaponBoxOwner
{
	GENERATED_BODY()

public:
	AIFMeleeEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Enemy|Components")
	virtual UBoxComponent* GetWeaponCollisionBox() const override { return WeaponCollisionBox; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SwordMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShieldMesh;
};
