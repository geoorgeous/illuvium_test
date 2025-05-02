#include "ArenaGameModeBase.h"

void AArenaGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	BattleSimulator.Init(BattleSimulationConfig, MakeShared<FAStarPathfinder>());
	
	TArray<FArenaBattleTile> Tiles;
	TArray<FArenaBattleUnitState> UnitStates;
	
	for (auto PathNodes = BattleSimulator.GetPathNodes(); const auto& PathNode : PathNodes)
	{
		Tiles.Add({
			.Coordinates = PathNode.Coordinates,
			.bIsTraversable = PathNode.bIsTraversable
		});
	}
	
	CollateBattleUnitStates(UnitStates);
	
	OnBattleBegin(Tiles, UnitStates);
}

void AArenaGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedSinceLastStep += DeltaSeconds;
	if (ElapsedSinceLastStep > TimeBetweenSteps)
	{
		BattleSimulator.Step();

		TArray<FArenaBattleUnitState> UnitStates;
		
		CollateBattleUnitStates(UnitStates);
		
		OnBattleStep(ElapsedSinceLastStep, UnitStates);
		
		ElapsedSinceLastStep = 0;
	}
}

void AArenaGameModeBase::CollateBattleUnitStates(TArray<FArenaBattleUnitState>& UnitStates) const
{
	UnitStates.Empty();
	
	for (auto& TeamStates = BattleSimulator.GetTeamStates(); auto& KeyVal : TeamStates)
		for (auto& Unit : KeyVal.Value.Units)
		{
			UnitStates.Add({
				.TeamLabel = KeyVal.Key,
				.Coordinates = Unit.PathNode->Coordinates,
				.HitPoints = Unit.HitPoints,
				.HitPointsMax = Unit.HitPointsMax
			});
		}
}

FVector3f AArenaGameModeBase::TileCoordinatesToWorldCoordinates(const FVector3f& TileCoordinates) const
{
	if (BattleSimulationConfig.bHexMap)
	{
		constexpr float HalfSizeScale = 0.65f;
		return {
			sqrtf(3.0f) * HalfSizeScale * (TileCoordinates.Z / 2 + TileCoordinates.X),
			1.5f * HalfSizeScale * TileCoordinates.Z,
			0
		};
	}
	
	return TileCoordinates;
}
