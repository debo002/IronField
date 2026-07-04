#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

class UAnimMontage;

// Clears any "on montage ended" callback bound to this montage, so a stale callback can't
// fire later against state that has already moved on.
inline void ClearMontageEndDelegate(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	if (!AnimInstance || !Montage)
	{
		return;
	}

	FOnMontageEnded EmptyDelegate;
	AnimInstance->Montage_SetEndDelegate(EmptyDelegate, Montage);
}