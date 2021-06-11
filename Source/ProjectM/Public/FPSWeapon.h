// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShakeBase;

UENUM()
enum class EWeaponSpeedType {
	SINGLE_SHOT,
	BURST_SHOT,
	AUTOMATIC,
};

UENUM()
enum class EWeaponShotType {
	RAYCAST,
	PROJECTILE,
};

USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class PROJECTM_API AFPSWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFPSWeapon();

	void StartFire();

	void EndFire();

	void ToggleScope();

	void Reload();

	void SetVisible(bool bIsVisible);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USkeletalMeshComponent* MeshComponent;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		EWeaponShotType ShotType;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		EWeaponSpeedType SpeedType;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		bool CanScopeIn;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "CanScopeIn"))
		float ScopeInAmount;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		int MagSize;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float MinHorizontalRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float MaxHorizontalRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float MinVerticalRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float MaxVerticalRecoil;

	//UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	//TSubclassOf<UCameraShakeBase> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "SpeedType == EWeaponSpeedType::BURST_SHOT", EditConditionHides))
		int RoundsPerBurst;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "SpeedType == EWeaponSpeedType::BURST_SHOT", EditConditionHides))
		float BurstDoneCooldown;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float ShotCooldownTime;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "ShotType == EWeaponShotType::PROJECTILE", EditConditionHides))
		TSubclassOf<AActor> ProjectileActor;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "ShotType == EWeaponShotType::RAYCAST", EditConditionHides))
		float ShotDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (EditCondition = "ShotType == EWeaponShotType::RAYCAST", EditConditionHides))
		float ShotDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		USoundBase* FireSound;



	UPROPERTY(VisibleDefaultsOnly, Category="FX")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, Category="FX")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
		UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
		UParticleSystem* TrailEffect;

private:
	FTimerHandle TimerHandle_WeaponCooldown;

	bool bCanShoot;
	bool bIsShooting;
	bool bIsScoped;

	int BurstCurrentShotCount;
	int AmmoCount;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
	void OnRep_HitScanTrace();

	void FireShot();

	UFUNCTION(Server, Reliable)
		void ServerFireShotRaycast();

	void FireShotRaycast();

	UFUNCTION(Server, Reliable)
		void ServerFireShotProjectile();

	UFUNCTION(Server, Reliable)
		void ChangeWeaponMesh();

	void FireShotProjectile();

	void RayCastFireEffects(FVector shootAt);

	void RayCastImpactEffect(FVector ImpactPoint, FVector TraceFrom, EPhysicalSurface SurfaceType);

	void ShotCooldown();
};