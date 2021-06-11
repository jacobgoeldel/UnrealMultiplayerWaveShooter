// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams(FOnHealthChangedSignature, UHealthComponent*, HealthComp, float, Health, float, MaxHealth, float, Shields, float, MaxShields, const class UDamageType*, DamageType, AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedClientSignature, float, Health, float, Shields);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	float GetHealth() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HealthComponent")
	uint8 TeamNum;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing=OnRep_HealthChanged, EditDefaultsOnly, BlueprintReadOnly, Category = "HealthComponent")
		float Health;

	UPROPERTY(ReplicatedUsing = OnRep_HealthChanged, EditDefaultsOnly, BlueprintReadOnly, Category = "HealthComponent")
		float Shields;

	UPROPERTY(BlueprintReadOnly, Category = "HealthComponent")
	float MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "HealthComponent")
		float MaxShields;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
		void OnRep_HealthChanged();

	UFUNCTION(NetMulticast, Reliable)
		void ShieldsPopped();

	UFUNCTION(NetMulticast, Reliable)
		void ShieldsBack();

	UFUNCTION(NetMulticast, Reliable)
		void Died();

public:

	UPROPERTY(BlueprintAssignable, Category = "Health")
		FOnHealthChangedClientSignature OnClientHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
		USoundBase* ShieldsPoppedSound;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
		USoundBase* ShieldsBackSound;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
		USoundBase* ShieldsChargingSound;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="HealthComponent")
	static bool IsFriendly(AActor* actor1, AActor* actor2);

private:
	FTimerHandle TimerHandle_RechargeShields;

	UAudioComponent* ShieldsChargingSoundComponent;

	void RechargeShields();
};
