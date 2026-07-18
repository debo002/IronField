#include "Character/IFCharacterSetupUtils.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

namespace IFCharacterSetupUtils
{
	void ConfigureWeaponCollisionBox(UBoxComponent* Box, USceneComponent* AttachParent, FName SocketName)
	{
		if (!Box || !AttachParent)
		{
			return;
		}

		Box->SetupAttachment(AttachParent, SocketName);
	}

	void ConfigureCosmeticEquipmentMesh(UStaticMeshComponent* Mesh, USceneComponent* AttachParent, FName SocketName)
	{
		if (!Mesh || !AttachParent)
		{
			return;
		}

		Mesh->SetupAttachment(AttachParent, SocketName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}
