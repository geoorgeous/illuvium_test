#pragma once

struct FPathNode
{
	FVector3f          Coordinates;
	bool               bIsTraversable;
	TArray<FPathNode*> Neighbors;

	// State for A* pathfinding algorithm.
	// todo: Better to move inside A* Pathfinding scope.
	FPathNode* Parent;
	float G;
	float H;
};

class FPathfinder
{
public:
	virtual ~FPathfinder() = default;

	/**
	 * Finds a contiguous path between @p Start and @p Target nodes.
	 * @param Start The path node to start the pathfinding from.
	 * @param Target The node to try and find a path to.
	 * @return An array of path nodes which link @p Start to @p Target.
	 * @note Both @p Start and @p Target nodes are excluded from the result, which means that if they are adjacent to
	 * each other, then the returned array will be empty. If no viable path from the @p Start node to the @p Target node
	 * can be found, then an empty array is returned. 
	 */
	virtual TArray<FPathNode*> FindPath(FPathNode& Start, FPathNode& Target) = 0;
};

class FAStarPathfinder final : public FPathfinder
{
public:

	/**
	 * Uses the A* pathfinding method to find a contiguous path between @p Start and @p Target nodes.
	 * @param Start The path node to start the pathfinding from.
	 * @param Target The node to try and find a path to.
	 * @return An array of path nodes which link @p Start to @p Target.
	 * @note Both @p Start and @p Target nodes are excluded from the result, which means that if they are adjacent to
	 * each other, then the returned array will be empty. If no viable path from the @p Start node to the @p Target node
	 * can be found, then an empty array is returned. 
	 */
	virtual TArray<FPathNode*> FindPath(FPathNode& Start, FPathNode& Target) override;
};