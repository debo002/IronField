#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "IFAnimNotifyStateAttackCollision.generated.h"

/**
 * Marks the swing window of an attack animation - collision turns on at NotifyBegin and off at
 * NotifyEnd. Carries no damage data; that lives on the combo step or spin attack instead.
 */
UCLASS()
class IRONFIELD_API UIFAnimNotifyStateAttackCollision : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};