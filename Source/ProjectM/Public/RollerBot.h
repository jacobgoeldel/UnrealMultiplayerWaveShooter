// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RollerBot.generated.h"

class UHealthComponent;
class URadialForceComponent;
class USphereComponent;

UCLASS()
class PROJECTM_API ARollerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARollerBot();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category="Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USphereComponent* SphereComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
		URadialForceComponent* ForceComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UHealthComponent *HealthComponent;

	UFUNCTION()
		void HandleTakeDamage(UHealthComponent* HealthComp, float Health, float MaxHealth, float Shields, float MaxShields, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	FVector NextPathPoint;

	void GetNextPathPoint();

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
		float DistancePointDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	bool bUseVelocityChange;

	UFUNCTION(NetMulticast, Reliable)
	void ExplodeSelf();

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
		USoundBase* ExplosionWarningSound;

	UPROPERTY(EditDefaultsOnly, Category = "Roller")
	USoundBase* ExplosionSound;

	UPROPERTY(BlueprintReadOnly, Category = "Roller")
	bool bExploded;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	bool bTimerStarted;
	FTimerHandle TimeHandler_SelfDamage;
	FTimerHandle TimeHandler_UpdatePathFinding;

	void DamageSelf();

};
