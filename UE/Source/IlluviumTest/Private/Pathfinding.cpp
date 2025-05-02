#include "Pathfinding.h"

TArray<FPathNode*> FAStarPathfinder::FindPath(FPathNode& Start, FPathNode& Target)
{
	if (Start.Coordinates == Target.Coordinates)
		return {};
	
	TArray<FPathNode*> PathResult = {};

	// List of nodes process.
	TArray<FPathNode*> OpenList = { &Start };

	// List of nodes that have been processed.
	TArray<FPathNode*> ClosedList;

	// Node that's being processed.
	FPathNode* Current = nullptr;
	
	bool bTargetFound = false;
	
	while (!OpenList.IsEmpty() && !bTargetFound)
	{
		// Find the node with the lowest (G + H) value to process next.
		Current = nullptr;
		for (auto i = 0; i < OpenList.Num(); i++)
		{
			if (Current == nullptr || OpenList[i]->G + OpenList[i]->H < Current->G + Current->H)
				Current = OpenList[i];		
		}

		// Mark as processed.
		OpenList.RemoveSingle(Current);
		ClosedList.Add(Current);

		for (FPathNode* NeighboringNode : Current->Neighbors)
		{
			// Stop if we're next to the 'Target' node.
			// This means we've found our path.
			if (NeighboringNode == &Target)
			{
				bTargetFound = true;
				break;
			}
			
			if (!NeighboringNode->bIsTraversable)
				continue;

			if (ClosedList.Contains(NeighboringNode))
				continue;

			// G = The distance from the 'Start' node.
			const float G = Current->G + FVector3f::DistSquared(NeighboringNode->Coordinates, Current->Coordinates);
			
			if (OpenList.Contains(NeighboringNode))
			{
				if (G < NeighboringNode->G)
				{
					NeighboringNode->Parent = Current;
					NeighboringNode->G = G;
				}
			}
			else
			{
				NeighboringNode->Parent = Current;
				NeighboringNode->G = G;
				// H = Our heuristic to steer the algorithm. Dead simple. Just the distance from the target.
				NeighboringNode->H = FVector3f::DistSquared(NeighboringNode->Coordinates, Target.Coordinates);
				
				OpenList.Add(NeighboringNode);
			}
		}
	}

	if (bTargetFound)
	{
		// At this point we have confirmed that there exists a continuous path from the 'Target' node all the way back to the
		// 'Start' node.
		// The way we construct our path here excludes the 'Start' and 'Target' nodes from the path list.
		// This means a few things:
		//  - If the 'Start' and 'Target' nodes are adjacent, then the path will be empty.
		//  - The first node in the path will be adjacent to the 'Start' node, NOT the 'Start' node itself.
		//  - The last node in the path will be adjacent to the 'Target' node, NOT the 'Target' node itself.
		
		while (Current != &Start)
		{
			PathResult.Add(Current);
			Current = Current->Parent;
		}
	
 		// Now we reverse the path, so that the first node in the array is adjacent to start.
		Algo::Reverse(PathResult);
	}

	return PathResult;
}
