#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

class UAnimMontage;

inline void ClearMontageEndDelegate(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	if (!AnimInstance || !Montage)
	{
		return;
	}

	FOnMontageEnded EmptyDelegate;
	AnimInstance->Montage_SetEndDelegate(EmptyDelegate, Montage);
}
