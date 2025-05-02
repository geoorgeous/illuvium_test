#pragma once

#include "Pathfinding.h"

#include "CoreMinimal.h"
#include "BattleSimulator.generated.h"

UENUM(BlueprintType)
enum class EBattleTeamLabel : uint8
{
	Blue,
	Red
};

USTRUCT(BlueprintType)
struct FBattleSimulationConfig
{
	GENERATED_USTRUCT_BODY()

	/**
	 * The value to seed the RandomStream with. If this value is 0 then the simulation will generate its own seed every
	 * time it is run.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Seed = 0;

	/**
	 * Half of the map's size. The full size of a map is calculated as (@p MapHalfSize * 2 + 1). For orthogonal maps
	 * this is the number of tiles on each axis. For hexagonal maps, this is the number of tiles from corner to corner. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MapHalfSize = 16;

	/**
	 * If true, then a hexagonal map with hexagonal tiles will be used for the simulation.
	 * If false, then a standard square orthogonal map with square tiles will be used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHexMap = true;

	/**
	 * The number of Units that spawn for each team at the start of the simulation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UnitsPerTeam = 12;

	/**
	 * The maximum number of tiles each unit can traverse in a single step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MovementPerSteps = 2;

	/**
	 * The maximum distance (in tiles) that a Unit can attack Targets from. For orthogonal maps, this includes diagonal
	 * tiles.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatRange = 1;

	/**
	 * Number of steps each Unit must wait in between consecutive attacks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackCooldown = 0;

	/**
	 * The minimum number of HP each unit is given. The total HP is calculated as (@p HitPointsBase + [0 ...
	 * @p HitPointsBonusMax]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HitPointsBase = 2;

	/**
	 * The maximum number of points each Unit is given on top of HitPointsBase. The total HP is calculated as
	 * (@p HitPointsBase + [0 ... @p HitPointsBonusMax]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HitPointsBonusMax = 3;

	/**
	 * The number of HitPoints each Unit deals to another when attacking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackPoints = 1;
};

struct FBattleSimulationTeamState;

struct FBattleUnitState
{
	const FBattleSimulationTeamState* Team;
	int32 HitPoints;
	int32 HitPointsOld;
	int32 HitPointsMax;
	FPathNode* PathNode;
	FBattleUnitState* Target;
	int32 StepsUntilNextAttack;
};

struct FBattleSimulationTeamState
{
	EBattleTeamLabel Label;
	TArray<FBattleUnitState> Units;
};


class FBattleSimulator
{
public:
	/**
	 * The default constructor which initializes a @c FBattleSimulator with the default configuration settings and an
	 * A* pathfinder.
	 */
	FBattleSimulator();

	/**
	 * Initializes the simulation to its starting state.
	 * @param ConfigIn The configuration details to use when setting up the simulation. See @c FBattleSimulationConfig
	 * members for details.
	 * @param PathfinderIn The pathfinder to use for Unit pathfinding.
	 */
	void Init(const FBattleSimulationConfig& ConfigIn, const TSharedPtr<FPathfinder>& PathfinderIn);

	/**
	 * Resets the simulation back to its starting state.
	 */
	void Reset();

	/**
	 * Steps the state of the simulation forward a discrete number of times.
	 * @param N The number of times to step the simulation forward.
	 * @note A single step consists of the following actions:
	 *  1. All Units try to acquire a target (the nearest 'enemy' Unit).
	 *  2. All Units try to find a path to their target and move along it.
	 *  3. All Units try to attack their target.
	 * Units with 0 or fewer HealthPoints are discarded from the above operations.
	 */
	void Step(int32 N = 1);

	/**
	 * Used to check whether the simulation is complete.
	 * @return True if there are no more actions for any Units to take.
	 */
	bool IsFinished() const;

	/**
	 * Returns the array of path nodes that exist within the simulation. 
	 */
	const TArray<FPathNode>& GetPathNodes() const;

	/**
	 * Returns the collections of Teams and their respective states that currently exist within the simulation. 
	 */
	const TMap<EBattleTeamLabel, FBattleSimulationTeamState>& GetTeamStates() const;

private:
	FBattleSimulationConfig Config;
	FRandomStream Random;
	TArray<FPathNode> PathNodes;
	TSharedPtr<FPathfinder> Pathfinder;
	TMap<EBattleTeamLabel, FBattleSimulationTeamState> TeamStates;

	void InitOrthogonalPathNodes();
	void InitHexagonalPathNodes();
	void InitUnit(FBattleUnitState& Unit, const FBattleSimulationTeamState& Team);

	void StepSingle();

	void AcquireTargetForUnit(FBattleUnitState& Unit);
	void DoUnitMovementAction(FBattleUnitState& Unit) const;
	void DoUnitAttackAction(FBattleUnitState& Unit) const;

	/**
	 * Simple helper function to run some logic on all Units across all teams, excluding the dead ones.
	 */
	template <typename TCallable>
	void ApplyToAllLivingUnits(TCallable Callable)
	{
		for (auto& KeyValue : TeamStates)
			for (auto& Unit : KeyValue.Value.Units)
				if (Unit.HitPointsOld > 0)
					Callable(Unit);
	}
};
