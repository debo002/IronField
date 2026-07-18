#pragma once

#include "CoreMinimal.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

namespace IFCharacterSetupUtils
{
	/** Shared weapon hitbox setup for player and melee enemies (socket, channels, disabled until attack window). */
	void ConfigureWeaponCollisionBox(UBoxComponent* Box, USceneComponent* AttachParent, FName SocketName = TEXT("weapon_r"));

	/** Shared cosmetic equipment mesh setup (no collision). */
	void ConfigureCosmeticEquipmentMesh(UStaticMeshComponent* Mesh, USceneComponent* AttachParent, FName SocketName);
}
