// Fill out your copyright notice in the Description page of Project Settings.


#include "PVEGameState.h"
#include "Net/UnrealNetwork.h"

void APVEGameState::SetWaveState(EWaveState NewState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		EWaveState oldState = WaveState;
		WaveState = NewState;
		OnRep_WaveState(oldState);
	}
}

void APVEGameState::OnRep_WaveState(EWaveState oldState)
{
	WaveStateChanged(WaveState, oldState);
	WaveStateChangedEvent.Broadcast(WaveState);
}

void APVEGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APVEGameState, WaveState);
	DOREPLIFETIME(APVEGameState, Wave);
}
