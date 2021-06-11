// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "FPSCharacter.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AFPSWeapon::AFPSWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->CastShadow = false;
	MeshComponent->bOnlyOwnerSee = true;
	RootComponent = MeshComponent;

	MuzzleSocketName = "MuzzleSocket";

	bCanShoot = true;
	bIsShooting = false;
	BurstCurrentShotCount = 0;
	bIsScoped = false;

	SetReplicates(true);
}

void AFPSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFPSWeapon::BeginPlay()
{
	AmmoCount = MagSize;
}

void AFPSWeapon::StartFire()
{
	if (AmmoCount > 0)
	{
		bIsShooting = true;
		if (bCanShoot)
		{
			if (SpeedType == EWeaponSpeedType::BURST_SHOT)
			{
				BurstCurrentShotCount = RoundsPerBurst;
			}

			bCanShoot = false;
			FireShot();
		}
	}
}

void AFPSWeapon::EndFire()
{
	bIsShooting = false;
}

void AFPSWeapon::SetVisible(bool bIsVisible)
{
	if (!bIsVisible)
	{
		bIsShooting = false;
		if (bIsScoped)
			ToggleScope();
	}
	else
	{
		AFPSCharacter* Player = Cast<AFPSCharacter>(GetOwner());
		if (Player)
		{
			Player->AmmoChanged(AmmoCount, MagSize);
		}
		ChangeWeaponMesh();
	}
	MeshComponent->SetVisibility(bIsVisible);
}

void AFPSWeapon::ToggleScope()
{
	if (CanScopeIn)
	{
		bIsScoped = !bIsScoped;
		MeshComponent->SetVisibility(!bIsScoped);

		AFPSCharacter* Player = Cast<AFPSCharacter>(GetOwner());
		if (Player)
		{
			if (bIsScoped)
			{
				Player->ChangeFov(ScopeInAmount);
			}
			else
			{
				Player->ChangeFov(1.0f);
			}
		}
	}
}

void AFPSWeapon::Reload()
{
	if (AmmoCount != MagSize)
	{
		AmmoCount = MagSize;

		if (bIsScoped)
			ToggleScope();

		AFPSCharacter* MyOwner = Cast<AFPSCharacter>(GetOwner());
		if (MyOwner)
		{
			MyOwner->AmmoChanged(AmmoCount, MagSize);
			MyOwner->ServerStartReloadAnimation();
		}
	}
}

void AFPSWeapon::OnRep_HitScanTrace()
{
	FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
	RayCastFireEffects(HitScanTrace.TraceTo);
	RayCastImpactEffect(HitScanTrace.TraceTo, MuzzleLocation, HitScanTrace.SurfaceType);
}

void AFPSWeapon::FireShot()
{
	AmmoCount--;

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	AFPSCharacter* MyOwner = Cast<AFPSCharacter>(GetOwner());
	if (MyOwner)
	{
		// recoil
		float VRec = FMath::RandRange(MinVerticalRecoil, MaxVerticalRecoil);
		float HRec = FMath::RandRange(MinHorizontalRecoil, MaxHorizontalRecoil);
		MyOwner->Recoil(HRec, VRec);

		MyOwner->StartFireAnimation();
		MyOwner->AmmoChanged(AmmoCount, MagSize);

		// cam shake
		/*APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientStartCameraShake(FireCamShake);
		}*/
	}

	if (ShotType == EWeaponShotType::RAYCAST)
		FireShotRaycast();
	if (ShotType == EWeaponShotType::PROJECTILE)
		FireShotProjectile();
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponCooldown, this, &AFPSWeapon::ShotCooldown, ShotCooldownTime);
}

void AFPSWeapon::ServerFireShotRaycast_Implementation()
{
	FireShotRaycast();
}

void AFPSWeapon::FireShotRaycast()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerFireShotRaycast();
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		FHitResult Hit;
		FVector StartTrace = MyOwner->GetPawnViewLocation();
		FVector ForwardVector = MyOwner->GetViewRotation().Vector();
		FVector EndTrace = (ForwardVector * 5000.0f) + StartTrace;
		FCollisionQueryParams* CQP = new FCollisionQueryParams();
		CQP->AddIgnoredActor(MyOwner);
		CQP->AddIgnoredActor(this);
		CQP->bReturnPhysicalMaterial = true;
		CQP->bTraceComplex = true;

		FVector TracerEndPoint = EndTrace;

		if (GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_GameTraceChannel1, *CQP))
		{
			AActor* HitActor = Hit.GetActor();

			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float DamageMult = 1.0f;
			if (SurfaceType == SurfaceType2)
			{
				DamageMult = 3.0f;
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ShotDamage * DamageMult, ForwardVector, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			if (GetLocalRole() == ROLE_Authority)
			{
				HitScanTrace.SurfaceType = SurfaceType;
			}

			FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
			RayCastImpactEffect(Hit.ImpactPoint, MuzzleLocation, SurfaceType);

			TracerEndPoint = Hit.ImpactPoint;
		}

		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
		}

		RayCastFireEffects(TracerEndPoint);
	}
}

void AFPSWeapon::ServerFireShotProjectile_Implementation()
{
	FireShotProjectile();
}

void AFPSWeapon::ChangeWeaponMesh_Implementation()
{
	AFPSCharacter* Player = Cast<AFPSCharacter>(GetOwner());
	if (Player)
	{
		Player->ChangeThirdPersonWeapon(MeshComponent->SkeletalMesh);
	}
}

void AFPSWeapon::FireShotProjectile()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerFireShotProjectile();
		return;
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner && ProjectileActor)
	{
		FVector StartLocation = MyOwner->GetPawnViewLocation();
		FRotator ForwardVector = MyOwner->GetViewRotation();
		StartLocation += (ForwardVector.Vector() * 100.0f);

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ActorSpawnParams.Instigator = Cast<APawn>(GetOwner());

		// spawn the projectile at the muzzle
		AActor* Projectile = GetWorld()->SpawnActor<AActor>(ProjectileActor, StartLocation, ForwardVector, ActorSpawnParams);
		Projectile->SetOwner(GetOwner());
	}
}

void AFPSWeapon::RayCastFireEffects(FVector shootAt)
{
	AFPSCharacter* PC = Cast<AFPSCharacter>(GetOwner());
	if (PC)
	{
		if (PC->IsLocallyControlled())
		{
			// Play Effects
			if (MuzzleEffect)
			{
				UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComponent, MuzzleSocketName);
			}

			if (TrailEffect)
			{
				FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
				UParticleSystemComponent* ParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TrailEffect, MuzzleLocation);
				if (ParticleComponent)
				{
					ParticleComponent->SetVectorParameter("BeamEnd", shootAt);
				}
			}
		}
		else
		{
			PC->FireEffectsThirdPerson(shootAt, MuzzleEffect, TrailEffect);
		}
		
	}
	
}

void AFPSWeapon::RayCastImpactEffect(FVector ImpactPoint, FVector TraceFrom, EPhysicalSurface SurfaceType)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SurfaceType1: // Flesh
		SelectedEffect = FleshImpactEffect;
		break;
	case SurfaceType2: // Vulnerable
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector ImpactNormal = TraceFrom - ImpactPoint;
		ImpactNormal.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ImpactNormal.Rotation());
	}
}

void AFPSWeapon::ShotCooldown()
{
	bCanShoot = true;
	switch (SpeedType)
	{
	case EWeaponSpeedType::BURST_SHOT:
		BurstCurrentShotCount--;
		if (BurstCurrentShotCount > 0 && AmmoCount > 0)
		{
			bCanShoot = false;
			FireShot();
		}
		else if (BurstCurrentShotCount == 0)
		{
			bCanShoot = false;
			GetWorldTimerManager().SetTimer(TimerHandle_WeaponCooldown, this, &AFPSWeapon::ShotCooldown, BurstDoneCooldown);
		}
		else 
		{
			bIsShooting = false;
			bCanShoot = true;
		}
		break;
	case EWeaponSpeedType::AUTOMATIC:
		if (bIsShooting && AmmoCount > 0)
		{
			FireShot();
		}
	default:
		break;
	}
}

void AFPSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFPSWeapon, HitScanTrace, COND_SkipOwner);
}

