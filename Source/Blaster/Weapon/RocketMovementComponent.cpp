// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketMovementComponent.h"

// We are using these two functions so that even when we hit ourselves we won't blow up but the projectile will move forward
UProjectileMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(
	const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// Super::HandleImpact(Hit, TimeSlice, MoveDelta);
	// Rockets should not stop; only explode when their CollisionBox detects a hit
}
