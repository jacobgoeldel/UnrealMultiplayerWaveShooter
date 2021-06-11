// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include "Components/AudioComponent.h"
#include "FPSCharacter.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	Health = 100.0f;
	Shields = 30.0f;

	TeamNum = 255;

	SetIsReplicatedByDefault(true);
}


float UHealthComponent::GetHealth() const
{
	return Health;
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
		}
	}

	MaxHealth = Health;
	MaxShields = Shields;
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
		return;

	if(DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser))
		return;

	if (Shields <= 0.0f)
	{
		Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	}
	else
	{
		Shields = FMath::Clamp(Shields - Damage, 0.0f, MaxShields);
		if (Shields <= 0.0f)
		{
			ShieldsPopped();
		}
	}

	if (Health <= 0.0f)
		Died();

	GetOwner()->GetWorldTimerManager().ClearTimer(TimerHandle_RechargeShields);
	GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle_RechargeShields, this, &UHealthComponent::RechargeShields, 3.5f);

	OnHealthChanged.Broadcast(this, Health, MaxHealth, Shields, MaxShields, DamageType, InstigatedBy, DamageCauser);
	OnClientHealthChanged.Broadcast(Health, Shields);
}

void UHealthComponent::OnRep_HealthChanged()
{
	OnClientHealthChanged.Broadcast(Health, Shields);
}

bool UHealthComponent::IsFriendly(AActor* actor1, AActor* actor2)
{
	if(actor1 == nullptr || actor2 == nullptr)
		return true;
	
	UHealthComponent* health1 = Cast<UHealthComponent>(actor1->GetComponentByClass(UHealthComponent::StaticClass()));
	UHealthComponent* health2 = Cast<UHealthComponent>(actor2->GetComponentByClass(UHealthComponent::StaticClass()));

	if(health1 == nullptr || health2 == nullptr)
		return true;

	return health1->TeamNum == health2->TeamNum;
}

void UHealthComponent::Died_Implementation()
{
	if (ShieldsChargingSoundComponent)
	{
		ShieldsChargingSoundComponent->Stop();
		ShieldsChargingSoundComponent->DestroyComponent();
	}
}

void UHealthComponent::ShieldsBack_Implementation()
{
	AFPSCharacter *PC = Cast<AFPSCharacter>(GetOwner());
	if (PC)
	{
		if (PC->IsLocallyControlled())
		{
			if (ShieldsChargingSoundComponent)
			{
				ShieldsChargingSoundComponent->DestroyComponent();
				ShieldsChargingSoundComponent = nullptr;
			}
			UGameplayStatics::PlaySound2D(PC, ShieldsBackSound);
		}
	}
	
}

void UHealthComponent::ShieldsPopped_Implementation()
{
	AFPSCharacter *PC = Cast<AFPSCharacter>(GetOwner());
	if (PC)
	{
		if (PC->IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(PC, ShieldsPoppedSound);
			ShieldsChargingSoundComponent = UGameplayStatics::SpawnSound2D(PC, ShieldsChargingSound);
		}
	}
}

void UHealthComponent::RechargeShields()
{
	Shields = MaxShields;
	
	ShieldsBack();
	OnHealthChanged.Broadcast(this, Health, MaxHealth, Shields, MaxShields, nullptr, nullptr, nullptr);
	OnClientHealthChanged.Broadcast(Health, Shields);
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, Shields);
}