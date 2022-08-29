// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"
#define TRACE_LENGTH 80000.f

class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* WeaponToEquip);	
protected:
	virtual void BeginPlay() override;

	void SetIsAiming(bool bAiming); //Server and client side
	
	UFUNCTION(Server, Reliable)
	void ServerSetIsAiming(bool bAiming); //Server will replicate to all clients
	
	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget); //FVector_NetQuantize is to reduce bandwidth across the network and to replicate the projectile's direction on all clients
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

private:
	class ABlasterCharacter* Character;
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;
public:	
	

		
};
