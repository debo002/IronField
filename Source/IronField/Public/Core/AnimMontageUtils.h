#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

class UAnimMontage;

/**
 * Small shared helpers for animation montage bookkeeping used by multiple classes
 * (character death/revive lifecycle, combo attacks, spin attacks) to avoid
 * duplicating the same delegate-clearing pattern in each one.
 */
namespace IF::AnimMontageUtils
{
	/** Clears any previously bound end delegate for Montage on AnimInstance, if both are valid. */
	inline void ClearMontageEndDelegate(UAnimInstance* AnimInstance, UAnimMontage* Montage)
	{
		if (!AnimInstance || !Montage)
		{
			return;
		}

		FOnMontageEnded EmptyDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyDelegate, Montage);
	}
}