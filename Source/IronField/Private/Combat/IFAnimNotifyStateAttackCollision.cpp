#include "Combat/IFAnimNotifyStateAttackCollision.h"

#include "Combat/IFCombatComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	UIFCombatComponent* FindOwnerCombat(USkeletalMeshComponent* MeshComp)
	{
		const AActor* const Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UIFCombatComponent>() : nullptr;
	}
}

void UIFAnimNotifyStateAttackCollision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UIFCombatComponent* const Combat = FindOwnerCombat(MeshComp))
	{
		Combat->BeginAttackCollision();
	}
}

void UIFAnimNotifyStateAttackCollision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UIFCombatComponent* const Combat = FindOwnerCombat(MeshComp))
	{
		Combat->EndAttackCollision();
	}
}
