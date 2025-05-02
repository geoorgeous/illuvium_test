#pragma once

#include "CoreMinimal.h"
#include "BattleSimulator.h"
#include "GameFramework/GameMode.h"
#include "ArenaGameModeBase.generated.h"

USTRUCT(BlueprintType)
struct FArenaBattleTile
{
	GENERATED_BODY()

	/**
	 * The tile-space coordinates of this tile. Use AArenaGameModeBase::TileCoordinatesToWorldCoordinates to convert
	 * these coordinates into coordinates to use in a level's world-space.
	 */
	UPROPERTY(BlueprintReadWrite)
	FVector3f Coordinates = FVector3f();

	/**
	 * True if this tile can be freely occupied by Units.
	 */
	UPROPERTY(BlueprintReadWrite)
	bool bIsTraversable = true;
};

USTRUCT(BlueprintType)
struct FArenaBattleUnitState
{
	GENERATED_BODY()

	/**
	 * The Team that the Unit belongs to. Other Units on the same team will not be considered for attacking.
	 */
	UPROPERTY(BlueprintReadWrite)
	EBattleTeamLabel TeamLabel = EBattleTeamLabel::Blue;

	/**
	 * The Unit's coordinates in the simulation in tile-space. Use AArenaGameModeBase::TileCoordinatesToWorldCoordinates
	 * to convert these coordinates into coordinates to use in a level's world-space.
	 */
	UPROPERTY(BlueprintReadWrite)
	FVector3f Coordinates = FVector3f();

	/**
	 * The Unit's HitPoints. If this value is 0 or lower, then the Unit is no longer active within the simulation.
	 */
	UPROPERTY(BlueprintReadWrite)
	int32 HitPoints = 0;

	/**
	 * The value of Unit's HitPoints when it was spawned in the simulation.
	 */
	UPROPERTY(BlueprintReadWrite)
	int32 HitPointsMax = 0;
};

UCLASS()
class AArenaGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	/**
	 * The configuration settings to use when initializing the simulation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBattleSimulationConfig BattleSimulationConfig;

	/**
	 * The duration of time (in seconds) to wait between steps of the simulation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeBetweenSteps = 0.5f;

	/**
	 * Converts a set of tile-space coordinates to level world-space coordinates. 
	 * @param TileCoordinates The coordinates in simulation tile-space. 
	 * @return The supplied tile-space coordinates converted to world-space coordinates.
	 */
	UFUNCTION(BlueprintPure)
	FVector3f TileCoordinatesToWorldCoordinates(const FVector3f& TileCoordinates) const;

	/**
	 * Triggered when the simulation has been initialized.
	 * @param Tiles The tiles that the simulation has been initialized with.
	 * @param UnitStates The initial states of the Units that have been spawned within the simulation.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBattleBegin(const TArray<FArenaBattleTile>& Tiles, const TArray<FArenaBattleUnitState>& UnitStates);

	/**
	 * Triggered when the simulation has been stepped forward once.
	 * @param DeltaSeconds The duration of time that has passed since the simulation was last stepped forward.
	 * @param UnitStates The new states of the Units that exist within the simulation.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBattleStep(float DeltaSeconds, const TArray<FArenaBattleUnitState>& UnitStates);
	
private:
	FBattleSimulator BattleSimulator;

	float ElapsedSinceLastStep;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	void CollateBattleUnitStates(TArray<FArenaBattleUnitState>& UnitStates) const;
};
