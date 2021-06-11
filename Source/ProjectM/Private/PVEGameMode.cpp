// Fill out your copyright notice in the Description page of Project Settings.


#include "PVEGameMode.h"
#include "TimerManager.h"
#include "HealthComponent.h"
#include "PVEGameState.h"
#include "PVEPlayerState.h"
#include "FPSCharacter.h"


APVEGameMode::APVEGameMode()
{
	TimeBetweenWaves = 7.0f;

	GameStateClass = APVEGameMode::StaticClass();
	PlayerStateClass = APVEPlayerState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

void APVEGameMode::StartWave()
{
	bGameOver = false;
	WaveCount++;

	APVEGameState* GS = GetGameState<APVEGameState>();
	if (ensureAlways(GS))
	{
		GS->Wave = WaveCount;
	}

	BotOn = 0;
	GetWorldTimerManager().SetTimer(TimeHandle_BotSpawner, this, &APVEGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);

	SetWaveState(EWaveState::WaveInProgress);
}

void APVEGameMode::StartPlay()
{
	Super::StartPlay();

	PrepareForNextWave();
}

void APVEGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckWaveState();
}

void APVEGameMode::PrepareForNextWave()
{
	SetWaveState(EWaveState::Preparing);
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &APVEGameMode::StartWave, TimeBetweenWaves, false);
}

void APVEGameMode::GameDone()
{
	bGameOver = true;

	EndWave();

	SetWaveState(EWaveState::Gameover);
}

void APVEGameMode::SpawnBotTimerElapsed()
{
	if (BotOn >= WavesInformation[WaveCount].EnemiesToSpawn.Num())
	{
		EndWave();
		return;
	}

	SpawnNewBot(WavesInformation[WaveCount].EnemiesToSpawn[BotOn]);

	BotOn++;
}

void APVEGameMode::CheckWaveState()
{
	if (BotOn < WavesInformation[WaveCount].EnemiesToSpawn.Num() || GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart) || bGameOver)
	{
		return;
	}

	bool bIsAnyBotAlive = false;
	bool bIsAnyPlayerAlive = false;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr)
		{
			continue;
		}

		bool bIsPlayer = false;

		if (TestPawn->IsPlayerControlled())
		{
			bIsPlayer = true;
		}

		UHealthComponent* HealthComp = Cast<UHealthComponent>(TestPawn->GetComponentByClass(UHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0)
		{
			if (bIsPlayer)
			{
				bIsAnyPlayerAlive = true;
			}
			else
			{
				bIsAnyBotAlive = true;
			}
		}
	}
	
	if (!bIsAnyBotAlive && bIsAnyPlayerAlive)
	{
		if(WaveCount + 1 < WavesInformation.Num())
		{
			PrepareForNextWave();
		}
		else
		{
			GameDone();
		}
	}

	if (!bIsAnyPlayerAlive)
	{
		GameOver();
	}
}

void APVEGameMode::GameOver()
{
	bGameOver = true;

	EndWave();

	SetWaveState(EWaveState::Gameover);

	// Finish up match, show game over
}

void APVEGameMode::SetWaveState(EWaveState NewState)
{
	APVEGameState* GS = GetGameState<APVEGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}

void APVEGameMode::EndWave()
{
	SetWaveState(EWaveState::WaitingToComplete);
	GetWorldTimerManager().ClearTimer(TimeHandle_BotSpawner);
}
