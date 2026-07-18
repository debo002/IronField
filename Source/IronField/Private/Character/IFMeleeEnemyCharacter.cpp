#include "Character/IFMeleeEnemyCharacter.h"

#include "Character/IFCharacterSetupUtils.h"
#include "Combat/IFMeleeCombatComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AIFMeleeEnemyCharacter::AIFMeleeEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIFMeleeCombatComponent>(TEXT("Combat")))
{
	CombatRange = 140.f;

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
	IFCharacterSetupUtils::ConfigureWeaponCollisionBox(WeaponCollisionBox, GetMesh());

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
	IFCharacterSetupUtils::ConfigureCosmeticEquipmentMesh(SwordMesh, GetMesh(), TEXT("weapon_r"));

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	IFCharacterSetupUtils::ConfigureCosmeticEquipmentMesh(ShieldMesh, GetMesh(), TEXT("weapon_l"));
}
