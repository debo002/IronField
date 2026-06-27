#include "IFAnimNotifyStateAttackCollision.h"

#include "Components/IFCombatComponent.h"
#include "GameFramework/Actor.h"

void UIFAnimNotifyStateAttackCollision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp || Damage <= 0.f)
    {
        return;
    }

    AActor* const Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    if (UIFCombatComponent* const Combat = Owner->FindComponentByClass<UIFCombatComponent>())
    {
        Combat->BeginAttackCollision(Damage, DamageTypeClass);
    }
}

void UIFAnimNotifyStateAttackCollision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AActor* const Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    if (UIFCombatComponent* const Combat = Owner->FindComponentByClass<UIFCombatComponent>())
    {
        Combat->EndAttackCollision();
    }
}
