// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PVEGameState.generated.h"

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	Preparing,
	WaveInProgress,
	WaitingToComplete,
	WaveComplete,
	Gameover,
	GameWon,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaveStateChangedSignature, EWaveState, NewState);

UCLASS()
class PROJECTM_API APVEGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void SetWaveState(EWaveState NewState);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GameState")
		int Wave;

	UPROPERTY(BlueprintAssignable, Category = "GameState")
	FWaveStateChangedSignature WaveStateChangedEvent;

protected:
	UFUNCTION()
	void OnRep_WaveState(EWaveState oldState);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveState, Category = "GameState")
		EWaveState WaveState;

	UFUNCTION(BlueprintImplementableEvent, category="GameState")
	void WaveStateChanged(EWaveState newState, EWaveState oldState);
	
};
