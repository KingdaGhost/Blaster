// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UBlasterAnimInstance::NativeInitializeAnimation() 
{
    Super::NativeInitializeAnimation();
    BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime) 
{
    Super::NativeUpdateAnimation(DeltaTime);
    if(BlasterCharacter == nullptr)
    {
        BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
    }
    if(BlasterCharacter == nullptr) return;

    FVector Velocity = BlasterCharacter->GetVelocity();
    Velocity.Z = 0.f;
    Speed = Velocity.Size();

    bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

    bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

    bIsWeaponEquipped = BlasterCharacter->IsWeaponEquipped();

    bIsCrouched = BlasterCharacter->bIsCrouched;

    bIsAiming = BlasterCharacter->IsAiming();

    //Offset Yaw for Strafing
    FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
    FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
    DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f); //Smooth out the strafing from left to right without any jittering
    YawOffset = DeltaRotation.Yaw;

    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = BlasterCharacter->GetActorRotation();
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);  //Difference between these two
    // UE_LOG(LogTemp, Warning, TEXT("Delta Yaw : %f"), Delta.Yaw);
    const float Target = Delta.Yaw / DeltaTime; //This is so that it will be frame rate independent
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f); // to ease out the lean so that there will be no jerkiness
    Lean = FMath::Clamp(Interp, -90.f, 90.f);
}
