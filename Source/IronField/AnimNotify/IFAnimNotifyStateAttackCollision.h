#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "IFAnimNotifyStateAttackCollision.generated.h"

class UDamageType;

/**
 * Animation notify state controlling weapon attack collision windows.
 * Directly interacts with the combat component to enable and disable active overlaps.
 */
UCLASS()
class IRONFIELD_API UIFAnimNotifyStateAttackCollision : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
    UPROPERTY(EditAnywhere, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true"))
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UDamageType> DamageTypeClass;
};

