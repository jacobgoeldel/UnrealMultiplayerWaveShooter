#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USoundBase;
class UAnimMontage;
class AFPSWeapon;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedClientSignature, int, Ammo, int, MaxAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

UCLASS()
class AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	/** Pawn mesh: 1st person view  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
		USkeletalMeshComponent* Mesh1PComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
		USkeletalMeshComponent* WeaponMeshComponent;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Health")
	UHealthComponent* HealthComponent;

public:
	AFPSCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
		TSubclassOf<AFPSWeapon> FirstWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
		TSubclassOf<AFPSWeapon> SecondWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
		TSubclassOf<APawn> SpectatingType;

	UPROPERTY(EditAnywhere, Category = "Controls")
		float MouseSens;

	UPROPERTY(EditAnywhere, Category = "Controls")
		float ControllerSensX;

	UPROPERTY(EditAnywhere, Category = "Controls")
		float ControllerSensY;

	UPROPERTY(EditAnywhere, Category = "Controls")
		float AimAssistSpeedReduction;

	void ChangeFov(float ScaleAmount);

	void Recoil(float HorizontalRecoil, float VerticalRecoil);

	void StartFireAnimation();

	UFUNCTION()
		void AmmoChanged(int Ammo, int MaxAmmo);

	UFUNCTION(Server, Reliable)
		void ServerStartReloadAnimation();

	UFUNCTION(NetMulticast, Reliable)
	void StartReloadAnimation();

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		UAnimMontage* FireAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		UAnimMontage* ReloadAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		UAnimMontage* ThirdPersonReloadAnimation;

	UFUNCTION(NetMulticast, Reliable)
		void ChangeThirdPersonWeapon(USkeletalMesh* NewMesh);

	void FireEffectsThirdPerson(FVector ShootAt, UParticleSystem *MuzzleEffect, UParticleSystem* TrailEffect);

	UPROPERTY(BlueprintAssignable, Category = "Health")
		FOnAmmoChangedClientSignature OnClientAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
		FOnDeathSignature OnDeathEvent;


protected:
	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles strafing movement, left and right */
	void MoveRight(float Val);

	void TurnMouse(float Val);

	void TurnController(float Val);

	void LookMouse(float Val);

	void LookController(float Val);

	void BeginCrouch();

	void EndCrouch();

	void ChangeWeapon();

	void ToggleScope();

	void Reload();

	bool CanHitTarget();

	UFUNCTION(Server, Reliable)
	void CreateInitialWeapons();

	UFUNCTION(NetMulticast, Reliable)
		void PlayerDie();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
		bool bAlive;

	UFUNCTION()
		void OnHealthChanged(UHealthComponent* HealthComp, float Health, float MaxHealth, float Shields, float MaxShields, const class UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void BeginPlay() override;

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1PComponent; }

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return CameraComponent; }

	UPROPERTY(ReplicatedUsing=OnRep_FirstWeaponActorChanged)
		AFPSWeapon* FirstWeaponActor;

	UPROPERTY(ReplicatedUsing=OnRep_SecondWeaponActorChanged)
		AFPSWeapon* SecondWeaponActor;

	UFUNCTION(BlueprintCallable, Category="Player")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category="Player")
	void StopFiring();


private:
	bool FirstWeaponCurrent;
	float DefaultFov;
	float FovScale;
	bool bHasCreatedWeapons;

	float RecoilAmtX;
	float RecoilAmtY;
	float RecoilSmoothAmt;

	UFUNCTION()
	void OnRep_SecondWeaponActorChanged();

	UFUNCTION()
		void OnRep_FirstWeaponActorChanged();

};

