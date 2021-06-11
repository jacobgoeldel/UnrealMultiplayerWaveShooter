// Fill out your copyright notice in the Description page of Project Settings.


#include "LaunchPad.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>

// Sets default values
ALaunchPad::ALaunchPad()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComp;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoxComp->SetBoxExtent(FVector(20.0f));
	BoxComp->SetupAttachment(MeshComp);
	BoxComp->SetGenerateOverlapEvents(true);

	BoxComp->OnComponentBeginOverlap.AddDynamic(this, &ALaunchPad::HandleOverlap);
}

void ALaunchPad::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		ACharacter* character = Cast<ACharacter>(OtherActor);
		if (character) {
			FRotator LaunchDirection = GetActorRotation();
			LaunchDirection.Pitch = LaunchPadAngle;
			FVector LaunchVector = LaunchDirection.Vector() * LaunchPadVelocity;
			character->LaunchCharacter(LaunchVector, true, true);
			UGameplayStatics::PlaySound2D(this, LaunchSound);
		}
	}
}


