#pragma once
#include <glm.hpp>

class Node;

/***************************************************************************//**
 * Plant handles all foliage calculations for a map using static functions.
 ******************************************************************************/
class Plant {
public:
	/***************************************************************************//**
	 * Root a plant at the given location. Will increase both the current and surrounding node foliage densities
	 @param nodes The nodes that make up the map data
	 @param dim The size of the map
	 @param pos The position to root from
	 @param foliageDensity The amount of foliage to place
	 ******************************************************************************/
    static void root(Node* nodes, glm::ivec2 dim, glm::ivec2 pos, float foliageDensity);
	/***************************************************************************//**
	 * Calculates fertility for the given node
	 @param node The node to check fertility of
	 ******************************************************************************/
    static float getFertilityForNode(const Node* node);
};