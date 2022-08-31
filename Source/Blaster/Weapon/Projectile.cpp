// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/Blaster.h"
#include "Net/UnrealNetwork.h"


AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); //because it will move(fly) around the world
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //for hit events
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //ignore all channels
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); //block the channel with visibility options
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);//and block the channel with WorldStatic options such as walls, trees, etc
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
	
	ProjectileMovementComponenet = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponenet->bRotationFollowsVelocity = true; //rotation align with velocity when drop off will follow
	
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	if(HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) //This is only called on the server
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	bIsCharacterHit = false;
	if(BlasterCharacter)
	{
		bIsCharacterHit = true;
		BlasterCharacter->MulticastHitReact(Hit.ImpactPoint); // The server will check and propagate to all clients
	}
	
	Destroy(); //This will call the Destroyed function which is like an RPC that will propagate all data from server to clients
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	
	if(bIsCharacterHit) return;
	
	if(MetalImpactParticles) //nullptr check for BlasterCharacter is for the Server. If this is not here then the below statements will run even if we hit the character
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MetalImpactParticles, GetActorTransform());
	}
	if(MetalImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MetalImpactSound, GetActorLocation());
	}
}


