#include "FPSCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PawnMovementComponent.h"
#include "FPSWeapon.h"
#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"


AFPSCharacter::AFPSCharacter()
{
	// Create a CameraComponent	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	CameraComponent->SetRelativeLocation(FVector(0, 0, BaseEyeHeight)); // Position the camera
	CameraComponent->bUsePawnControlRotation = true;

	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->SetOwnerNoSee(true);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1PComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh1PComponent->SetupAttachment(CameraComponent);
	Mesh1PComponent->CastShadow = false;
	Mesh1PComponent->SetRelativeRotation(FRotator(2.0f, -15.0f, 5.0f));
	Mesh1PComponent->SetRelativeLocation(FVector(0, 0, -160.0f));
	Mesh1PComponent->bOnlyOwnerSee = true;

	// Create a "dummy" weapon mesh for showing the weapon to other players
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->bCastHiddenShadow = true;
	WeaponMeshComponent->SetOwnerNoSee(true);
	WeaponMeshComponent->SetupAttachment(GetMesh(), "hand_rSocket");

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("PlayerHealthComponent"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &AFPSCharacter::OnHealthChanged);

	DefaultFov = CameraComponent->FieldOfView;
	FovScale = 1.0f;
	RecoilAmtX = 0.0f;
	RecoilAmtY = 0.0f;
	RecoilSmoothAmt = 0.1f;
	bAlive = true;
	bHasCreatedWeapons = false;
}


void AFPSCharacter::ChangeFov(float ScaleAmount)
{
	FovScale = ScaleAmount;
	CameraComponent->FieldOfView = DefaultFov * ScaleAmount;
}

void AFPSCharacter::Recoil(float HorizontalRecoil, float VerticalRecoil)
{
	RecoilAmtX += HorizontalRecoil * 0.01f;
	RecoilAmtY += VerticalRecoil * 0.01f;
}

void AFPSCharacter::StartFireAnimation()
{
	if (FireAnimation)
	{
		Mesh1PComponent->GetAnimInstance()->Montage_Play(FireAnimation);
	}
}

void AFPSCharacter::StartReloadAnimation_Implementation()
{
	if (ReloadAnimation)
	{
		Mesh1PComponent->GetAnimInstance()->Montage_Play(ReloadAnimation);
	}
	if (ThirdPersonReloadAnimation)
	{
		GetMesh()->GetAnimInstance()->Montage_Play(ThirdPersonReloadAnimation);
	}
}

void AFPSCharacter::AmmoChanged(int Ammo, int MaxAmmo)
{
	OnClientAmmoChanged.Broadcast(Ammo, MaxAmmo);
}

void AFPSCharacter::ServerStartReloadAnimation_Implementation()
{
	StartReloadAnimation();
}

void AFPSCharacter::FireEffectsThirdPerson(FVector ShootAt, UParticleSystem* MuzzleEffect, UParticleSystem* TrailEffect)
{
	// Play Effects
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, WeaponMeshComponent, "MuzzleSocket");
	}

	if (TrailEffect)
	{
		FVector MuzzleLocation = WeaponMeshComponent->GetSocketLocation("MuzzleSocket");
		UParticleSystemComponent* ParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TrailEffect, MuzzleLocation);
		if (ParticleComponent)
		{
			ParticleComponent->SetVectorParameter("BeamEnd", ShootAt);
		}
	}
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFPSCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AFPSCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCharacter::StartFiring);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AFPSCharacter::StopFiring);

	PlayerInputComponent->BindAction("ToggleScope", IE_Pressed, this, &AFPSCharacter::ToggleScope);
	PlayerInputComponent->BindAction("ChangeWeapon", IE_Pressed, this, &AFPSCharacter::ChangeWeapon);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFPSCharacter::Reload);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);

	PlayerInputComponent->BindAxis("TurnMouse", this, &AFPSCharacter::TurnMouse);
	PlayerInputComponent->BindAxis("TurnRateMouse", this, &AFPSCharacter::LookMouse);

	PlayerInputComponent->BindAxis("TurnController", this, &AFPSCharacter::TurnController);
	PlayerInputComponent->BindAxis("TurnRateController", this, &AFPSCharacter::LookController);

	FirstWeaponCurrent = true;
}


void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSCharacter::ChangeThirdPersonWeapon_Implementation(USkeletalMesh* NewMesh)
{
	WeaponMeshComponent->SetSkeletalMesh(NewMesh);
}

void AFPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bHasCreatedWeapons && (IsLocallyControlled() || IsBotControlled()))
	{
		bHasCreatedWeapons = true;
		CreateInitialWeapons();
	}

	AddControllerYawInput(RecoilAmtX);
	AddControllerPitchInput(RecoilAmtY);
	RecoilAmtX = FMath::Lerp(RecoilAmtX, 0.0f, RecoilSmoothAmt);
	RecoilAmtY = FMath::Lerp(RecoilAmtY, 0.0f, RecoilSmoothAmt);
}

void AFPSCharacter::OnRep_SecondWeaponActorChanged()
{
	if (SecondWeaponActor)
	{
		SecondWeaponActor->SetVisible(false);
	}
}

void AFPSCharacter::OnRep_FirstWeaponActorChanged()
{
	if (FirstWeaponActor)
	{
		FirstWeaponActor->SetVisible(true);
	}
}

void AFPSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}


void AFPSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSCharacter::TurnMouse(float Val)
{
	AddControllerYawInput(Val * MouseSens * FovScale);
}

void AFPSCharacter::TurnController(float Val)
{
	float multiplier = 1.0f;
	if (CanHitTarget())
		multiplier = AimAssistSpeedReduction;

	AddControllerYawInput(Val * ControllerSensX * FovScale * multiplier);
}

void AFPSCharacter::LookMouse(float Val)
{
	AddControllerPitchInput(Val * MouseSens * FovScale);
}

void AFPSCharacter::LookController(float Val)
{
	float multiplier = 1.0f;
	if (CanHitTarget())
		multiplier = AimAssistSpeedReduction;

	AddControllerPitchInput(Val * ControllerSensY * FovScale * multiplier);
}

void AFPSCharacter::BeginCrouch()
{
	Crouch();
}

void AFPSCharacter::EndCrouch()
{
	UnCrouch();
}

void AFPSCharacter::ChangeWeapon()
{
	FirstWeaponCurrent = !FirstWeaponCurrent;
	FirstWeaponActor->SetVisible(FirstWeaponCurrent);
	SecondWeaponActor->SetVisible(!FirstWeaponCurrent);
}

void AFPSCharacter::StartFiring()
{
	if (FirstWeaponCurrent && FirstWeaponActor)
	{
		FirstWeaponActor->StartFire();
	}
	else if(SecondWeaponActor)
	{
		SecondWeaponActor->StartFire();
	}
}

void AFPSCharacter::StopFiring()
{
	if (FirstWeaponCurrent && FirstWeaponActor)
	{
		FirstWeaponActor->EndFire();
	}
	else if(SecondWeaponActor)
	{
		SecondWeaponActor->EndFire();
	}
}

void AFPSCharacter::ToggleScope()
{
	if (FirstWeaponCurrent)
	{
		FirstWeaponActor->ToggleScope();
	}
	else
	{
		SecondWeaponActor->ToggleScope();
	}
}

void AFPSCharacter::Reload()
{
	if (FirstWeaponCurrent)
	{
		FirstWeaponActor->Reload();
	}
	else
	{
		SecondWeaponActor->Reload();
	}
}

bool AFPSCharacter::CanHitTarget()
{
	FHitResult Hit;
	FVector StartTrace = GetPawnViewLocation();
	FVector ForwardVector = GetViewRotation().Vector();
	FVector EndTrace = (ForwardVector * 5000.0f) + StartTrace;
	FCollisionQueryParams* CQP = new FCollisionQueryParams();
	CQP->AddIgnoredActor(this);
	CQP->bReturnPhysicalMaterial = false;
	CQP->bTraceComplex = true;

	if (GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_GameTraceChannel1, *CQP))
	{
		if (Hit.GetActor())
		{
			APawn* Pawn = Cast<APawn>(Hit.GetActor());
			if (Pawn)
			{
				return true;
			}
		}
	}

	return false;
}

void AFPSCharacter::PlayerDie_Implementation()
{
	if (FirstWeaponActor)
	{
		FirstWeaponActor->Destroy();
	}

	if (SecondWeaponActor)
	{
		SecondWeaponActor->Destroy();
	}
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	OnDeathEvent.Broadcast();
}

void AFPSCharacter::CreateInitialWeapons_Implementation()
{
	// Spawn Weapons
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FirstWeaponActor = GetWorld()->SpawnActor<AFPSWeapon>(FirstWeapon, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (FirstWeaponActor)
	{
		FirstWeaponActor->SetOwner(this);
		FirstWeaponActor->AttachToComponent(Mesh1PComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, "GripPoint");
		ChangeThirdPersonWeapon(FirstWeaponActor->MeshComponent->SkeletalMesh);
		FirstWeaponActor->SetVisible(true);
	}

	SecondWeaponActor = GetWorld()->SpawnActor<AFPSWeapon>(SecondWeapon, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (SecondWeaponActor)
	{
		SecondWeaponActor->SetOwner(this);
		SecondWeaponActor->AttachToComponent(Mesh1PComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, "GripPoint");
		SecondWeaponActor->SetVisible(false);
	}
}

void AFPSCharacter::OnHealthChanged(UHealthComponent* HealthComp, float Health, float MaxHealth, float Shields, float MaxShields, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && bAlive)
	{
		bAlive = false;
		GetMovementComponent()->StopMovementImmediately();
		SetLifeSpan(20.0f);

		PlayerDie();

		if (Controller)
		{
			StopFiring();
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			APawn* Spectator = GetWorld()->SpawnActor<APawn>(SpectatingType, GetActorLocation(), GetActorRotation(), SpawnParams);
			if (Spectator)
			{
				APlayerController* Contr = GetWorld()->GetFirstPlayerController();
					Controller->Possess(Spectator);
			}
		}
		
	}
}

void AFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSCharacter, FirstWeaponActor);
	DOREPLIFETIME(AFPSCharacter, SecondWeaponActor);
	DOREPLIFETIME(AFPSCharacter, bAlive);
}
