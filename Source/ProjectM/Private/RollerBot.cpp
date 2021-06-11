// Fill out your copyright notice in the Description page of Project Settings.


#include "RollerBot.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "HealthComponent.h"
#include "FPSCharacter.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"

// Sets default values
ARollerBot::ARollerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject <UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComponent->SetSimulatePhysics(true);
	RootComponent = MeshComponent;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComponent->SetSphereRadius(200.0f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject <UHealthComponent>(TEXT("HealthComp"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &ARollerBot::HandleTakeDamage);

	ForceComponent = CreateDefaultSubobject<URadialForceComponent>(TEXT("ForceComponent"));
	ForceComponent->SetupAttachment(RootComponent);

	MovementForce = 1000.0f;
	bUseVelocityChange = false;
	DistancePointDistance = 200.0f;
	bExploded = false;
	ExplosionDamage = 100.0f;
	ExplosionRadius = 200.0f;
	bTimerStarted = false;
}

void ARollerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	AFPSCharacter* PC = Cast<AFPSCharacter>(OtherActor);
	if (PC && !bTimerStarted && !bExploded && !UHealthComponent::IsFriendly(this, PC))
	{
		bTimerStarted = true;
		UGameplayStatics::SpawnSoundAttached(ExplosionWarningSound, RootComponent);

		if (GetLocalRole() == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimeHandler_SelfDamage, this, &ARollerBot::DamageSelf, 0.5f, true, 0.0f);
		}
	}
}

// Called when the game starts or when spawned
void ARollerBot::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		GetWorldTimerManager().SetTimer(TimeHandler_UpdatePathFinding, this, &ARollerBot::GetNextPathPoint, 3.0f, true, 0.0f);
	}
}

void ARollerBot::HandleTakeDamage(UHealthComponent* HealthComp, float Health, float MaxHealth, float Shields, float MaxShields, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (GetLocalRole() == ROLE_Authority && Health <= 0.1f)
	{
		ExplodeSelf();
	}
}

void ARollerBot::GetNextPathPoint()
{
	AFPSCharacter* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		AFPSCharacter* PC = Cast<AFPSCharacter>(TestPawn);

		if (PC && !UHealthComponent::IsFriendly(this, PC))
		{
			float Distance = (PC->GetActorLocation() - GetActorLocation()).Size();
			if (NearestTargetDistance > Distance)
			{
				BestTarget = PC;
				NearestTargetDistance = Distance;
			}
		}
	}

	if (BestTarget)
	{
		UNavigationPath *NewPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		if (NewPath->PathPoints.Num() > 1)
		{
			NextPathPoint = NewPath->PathPoints[1];
		}
	}
	else
	{
		NextPathPoint = GetActorLocation();
	}
}

void ARollerBot::ExplodeSelf_Implementation()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetVisibility(false, true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForceComponent->FireImpulse();

	if (GetLocalRole() == ROLE_Authority)
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), false);
		SetLifeSpan(1.0f);
	}
}

// Called every frame
void ARollerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();
		if (DistanceToTarget > DistancePointDistance)
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			// Move To Target
			MeshComponent->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
		}
		else
		{
			GetNextPathPoint();
			GetWorldTimerManager().ClearTimer(TimeHandler_UpdatePathFinding);
			GetWorldTimerManager().SetTimer(TimeHandler_UpdatePathFinding, this, &ARollerBot::GetNextPathPoint, 3.0f, true, 0.0f);
		}
	}
}

void ARollerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 10, GetInstigatorController(), this, nullptr);
}

