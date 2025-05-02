#include "BattleSimulator.h"

FBattleSimulator::FBattleSimulator()
{
	Init(FBattleSimulationConfig{}, MakeShared<FAStarPathfinder>());
}

void FBattleSimulator::Init(const FBattleSimulationConfig& ConfigIn, const TSharedPtr<FPathfinder>& PathfinderIn)
{
	this->Config = ConfigIn;
	this->Pathfinder = PathfinderIn;
	Reset();
}

void FBattleSimulator::Reset()
{
	// Here a Seed value of 0 is a special value indicating that the seed is random every time the simulation is set up.
	// If the value is not 0, then we use a specific seed to seed our randomness generator. This allows for our
	// simulation to be deterministic as long as it is initialised with the same configuration.
	if (Config.Seed == 0)
		Random.GenerateNewSeed();
	else
		Random.Initialize(Config.Seed);

	PathNodes = TArray<FPathNode>();

	if (Config.bHexMap)
		InitHexagonalPathNodes();
	else
		InitOrthogonalPathNodes();

	TeamStates = TMap<EBattleTeamLabel, FBattleSimulationTeamState>({
		{EBattleTeamLabel::Blue, {}},
		{EBattleTeamLabel::Red, {}}
		});

	// A simulation with less path nodes than Units makes little sense. Don't bother continuing initialization if this
	// is the case.
	if (TeamStates.Num() * Config.UnitsPerTeam > PathNodes.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize Battle Simulation: The map is not big enough for the number of Units specified. Please increase the map size or decrease the number of Units per team."));
		return;
	}
	
	for (auto& KeyVal : TeamStates)
	{
		KeyVal.Value.Label = KeyVal.Key;
		KeyVal.Value.Units.SetNum(Config.UnitsPerTeam);

		for (auto& ToInit :KeyVal.Value.Units)
			InitUnit(ToInit, KeyVal.Value);
	}
}

void FBattleSimulator::Step(int32 N)
{
	for (; N > 0; --N)
		StepSingle();
}

bool FBattleSimulator::IsFinished() const
{
	int32 TeamsRemaining = 0;
	
	for (auto& KeyVal : TeamStates)
	{
		bool bAllUnitsDead = true;
		
		for (const auto& Unit : KeyVal.Value.Units)
			if (Unit.HitPoints > 0)
			{
				bAllUnitsDead = false;
				break;
			}

		if (!bAllUnitsDead)
			TeamsRemaining++;
	}

	// If there aren't at least 2 teams remaining then the simulation can be considered complete, as there are no more
	// meaningful actions for units to take.
	return TeamsRemaining > 1;
}

const TArray<FPathNode>& FBattleSimulator::GetPathNodes() const
{
	return PathNodes;
}

const TMap<EBattleTeamLabel, FBattleSimulationTeamState>& FBattleSimulator::GetTeamStates() const
{
	return TeamStates;
}

void FBattleSimulator::InitOrthogonalPathNodes()
{
	const int32 MapSize = Config.MapHalfSize * 2 + 1;
	
	PathNodes.SetNum(MapSize * MapSize);

	for (int32 X = 0; X < MapSize; ++X)
	{
		for (int32 Y = 0; Y < MapSize; ++Y)
		{
			FPathNode& PathNode = PathNodes[X * MapSize + Y];
			
			PathNode = {
				.Coordinates = FVector3f(X - Config.MapHalfSize, Y - Config.MapHalfSize, 0),
				.bIsTraversable = true
			};

			// Connect the PathNodes to each other to create our navigation network.
			
			if (X > 0)
				PathNode.Neighbors.Add(&PathNodes[(X - 1) * MapSize + Y]);
			
			if (X < MapSize - 1)
				PathNode.Neighbors.Add(&PathNodes[(X + 1) * MapSize + Y]);

			if (Y > 0)
				PathNode.Neighbors.Add(&PathNodes[X * MapSize + Y - 1]);
			
			if (Y < MapSize - 1)
				PathNode.Neighbors.Add(&PathNodes[X * MapSize + Y + 1]);

			// If we wanted to disable diagonal movement for the orthogonal grid, we would simply skip adding these
			// nodes to the Neighbors array.

			if (X > 0 && Y > 0)
				PathNode.Neighbors.Add(&PathNodes[(X - 1) * MapSize + (Y - 1)]);

			if (X < MapSize - 1 && Y > 0)
				PathNode.Neighbors.Add(&PathNodes[(X + 1) * MapSize + (Y - 1)]);

			if (X > 0 && Y < MapSize - 1)
				PathNode.Neighbors.Add(&PathNodes[(X - 1) * MapSize + (Y + 1)]);

			if (X < MapSize - 1 && Y > 0 && Y < MapSize - 1)
				PathNode.Neighbors.Add(&PathNodes[(X + 1) * MapSize + (Y + 1)]);
		}
	}
}

void FBattleSimulator::InitHexagonalPathNodes()
{
	const int32 N = Config.MapHalfSize;

	TMap<FIntVector2, int32> AxialCoordsToIndex;

	for (int32 Q = -N; Q <= N; ++Q)
	{
		for (int32 R = FMath::Max(-N, -Q - N); R <= FMath::Min(N, -Q + N); ++R)
		{
			// We use full 3D coordinates here instead of two-component axial coordinates.
			// This just makes it easier to do spacial calculations later on when we're doing pathfinding, without
			// the pathfinding algorithm caring about the type of coordinates it's dealing with.
			AxialCoordsToIndex.Add({Q, R}, PathNodes.Add({
				.Coordinates = FVector3f(Q, R, -Q - R),
				.bIsTraversable = true
			}));
		}
	}

	// Connect the PathNodes to each other to create our navigation network.
	
	for (int32 Q = -N; Q <= N; ++Q)
	{
		for (int32 R = FMath::Max(-N, -Q - N); R <= FMath::Min(N, -Q + N); ++R)
		{
			FPathNode& PathNode = PathNodes[AxialCoordsToIndex[{Q, R}]];

			const int32 S = -Q - R;
			
			if (Q > -N && R < N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q - 1, R + 1}]]);

			if (Q < N && R > -N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q + 1, R - 1}]]);

			if (Q > -N && S < N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q - 1, R}]]);

			if (Q < N && S > -N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q + 1, R}]]);

			if (R > -N && S < N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q, R - 1}]]);

			if (R < N && S > -N)
				PathNode.Neighbors.Add(&PathNodes[AxialCoordsToIndex[{Q, R + 1}]]);
		}
	}
}

void FBattleSimulator::InitUnit(FBattleUnitState& Unit, const FBattleSimulationTeamState& Team)
{
	const int32 HitPoints = Config.HitPointsBase + Random.RandHelper(Config.HitPointsBonusMax + 1);
	
	Unit = {
		.Team = &Team,
		.HitPoints = HitPoints,
		.HitPointsOld = HitPoints,
		.HitPointsMax = HitPoints
	};

	// Currently any PathNode is a viable spawn node.
	// Note: Perhaps we might want to only spawn Units on PathNodes that are traversable. Though this would create
	// cases where spawning fails at this point which would have to be accounted for.
	
	Unit.PathNode = &PathNodes[Random.RandHelper(PathNodes.Num())];
	Unit.PathNode->bIsTraversable = false;
}

void FBattleSimulator::StepSingle()
{
	// Each action is completed for every Unit before moving on to the next one. In other words; all Units acquire a
	// target, then all Units move, and then finally all Units attempt to attack their targets.
	
	ApplyToAllLivingUnits([this](FBattleUnitState& Unit){
		AcquireTargetForUnit(Unit);
	});

	ApplyToAllLivingUnits([this](FBattleUnitState& Unit){
		DoUnitMovementAction(Unit);
	});
	
	ApplyToAllLivingUnits([this](FBattleUnitState& Unit){
		DoUnitAttackAction(Unit);
	});

	ApplyToAllLivingUnits([](FBattleUnitState& Unit){
		Unit.HitPointsOld = Unit.HitPoints;
	});
}

void FBattleSimulator::AcquireTargetForUnit(FBattleUnitState& Unit)
{
	// Reset target every time. Every step we check for the most appropriate target.
	Unit.Target = nullptr;
	
	for (auto& KeyVal : TeamStates)
	{
		// Don't consider Units on the same team as us.
		if (KeyVal.Key == Unit.Team->Label)
			continue;

		for (auto& EnemyUnit : KeyVal.Value.Units)
		{
			// Don't consider Units that are dead.
			if (EnemyUnit.HitPoints <= 0)
				continue;
			
			// We'll acquire this target if we don't already have one, or if this target is closer than our current one.
			if (Unit.Target == nullptr || FVector3f::DistSquared(Unit.PathNode->Coordinates, EnemyUnit.PathNode->Coordinates) < FVector3f::DistSquared(Unit.PathNode->Coordinates, Unit.Target->PathNode->Coordinates))
				Unit.Target = &EnemyUnit;
		}
	}
}

void FBattleSimulator::DoUnitMovementAction(FBattleUnitState& Unit)const
{
	// Not interested in moving if we don't have a target.
	if (Unit.Target == nullptr)
		return;

	// Try and path to our target. Since occupied tiles are flagged as non-traversable, this might fail.
	// This might happen if the target Unit is surrounded by other Units which have effectively walled-off the target.
	// A more effective and offensive method might be to make a valid path a condition of selecting a target. Though
	// this would incur an extra cost during the target-acquisition stage.
	TArray<FPathNode*> Path = Pathfinder->FindPath(*Unit.PathNode, *Unit.Target->PathNode);

	if (Path.IsEmpty())
		return;

	// Since this is a discrete simulation, we just relocate our unit to the end of the path for this step.
	// This does mean that fractional movement speeds (e.g, 0.5 tiles per step, 2.25 tiles per step) are nonsensical.
	// This follows the predefined rule that Units must always be located on a single discrete tile.
	const auto PathDestIndex = (Path.Num() < Config.MovementPerSteps ? Path.Num() : Config.MovementPerSteps) - 1;

	// Un-occupy old node and occupy new one.
	Unit.PathNode->bIsTraversable = true;
	Unit.PathNode = Path[PathDestIndex];
	Unit.PathNode->bIsTraversable = false;
}

void FBattleSimulator::DoUnitAttackAction(FBattleUnitState& Unit) const
{
	// Obviously we must do nothing if there's no target.
	if (Unit.Target == nullptr)
		return;
	
	// Must wait for cooldown to complete before we can attack.
	if (Unit.StepsUntilNextAttack > 0)
	{
		Unit.StepsUntilNextAttack--;
		return;
	}
	
	// Target must be in range before we can perform an attack. Otherwise, we do nothing.
	// Note: This is not a distance check. Since our range is measured in tiles and Units can move diagonally,
	// then a range of 1 means that diagonal grid squares are equally accessible as orthogonal ones.
	if ((Unit.Target->PathNode->Coordinates - Unit.PathNode->Coordinates).GetAbsMax() > Config.CombatRange)
		return;
	
	Unit.Target->HitPoints -= Config.AttackPoints;

	Unit.StepsUntilNextAttack = Config.AttackCooldown;
}