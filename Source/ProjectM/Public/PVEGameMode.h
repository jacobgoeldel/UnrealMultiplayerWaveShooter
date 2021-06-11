// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PVEGameMode.generated.h"

enum class EWaveState : uint8;

USTRUCT(BlueprintType)
struct FWaveInformation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Bots")
	TArray<TSubclassOf<APawn>> EnemiesToSpawn;
};

UCLASS()
class PROJECTM_API APVEGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	APVEGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Gamemode")
	float TimeBetweenWaves;

	UPROPERTY(EditDefaultsOnly, Category = "Gamemode")
	TArray<FWaveInformation> WavesInformation;


	UFUNCTION(BlueprintImplementableEvent, Category = "Gamemode")
	void SpawnNewBot(TSubclassOf<APawn> BotToSpawn);

	void SpawnBotTimerElapsed();

	void StartWave();

	void EndWave();

	void PrepareForNextWave();

	void GameDone();
	
	void CheckWaveState();

	void GameOver();

	void SetWaveState(EWaveState NewState);

private:
	FTimerHandle TimeHandle_BotSpawner;
	FTimerHandle TimerHandle_NextWaveStart;
	int32 BotOn;
	int32 WaveCount;
	bool bGameOver;

};
