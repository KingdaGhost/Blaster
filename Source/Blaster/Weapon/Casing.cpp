// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"

#include "Engine/EngineTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"


ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true); //Simulation generates HitEvents
	ShellEjectionImpulse = 10.f;
	ShellExpiryTime = 5.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	CasingMesh->AddLocalRotation(UKismetMathLibrary::RandomRotator());
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ACasing::DelayShellExpiry, ShellExpiryTime);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if(ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
	CasingMesh->SetNotifyRigidBodyCollision(false); //This is set to false so that the OnHit() will not play more than once
}

void ACasing::DelayShellExpiry()
{
	Destroy();
}



