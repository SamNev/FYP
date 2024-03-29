#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <SDL2/SDL.h>
#include <string>
#include <time.h>
#include <Windows.h>

#include "Node.h"
#include "Plant.h"

//#define FLOODTESTMAP
#define BEDROCK_LAYER 0.0f
#define BEDROCK_SAFETY_LAYER 0.1f

class PerlinNoise;

/***************************************************************************//**
 * Defines for the type of noise within the array of generated noise. Mostly
 * for code readability purposes.
 ******************************************************************************/
enum noiseType : int
{
	NoiseType_BaseVariance,
	NoiseType_Hill,
	NoiseType_Divet,
	NoiseType_Mountain,
	NoiseType_Lie,
	NoiseType_Rock,
	NoiseType_Resistivity,
	NoiseType_Sand,
};

/***************************************************************************//**
 * MapParams define all tweakable values for the program. Loaded from a map config
 * file (named "params" by defaut)
 ******************************************************************************/
struct MapParams
{
	void loadFromFile();

	/***************************************************************************//**
	 * Defines all properties in a map by their name. This is so they can be set
	 * when the properties are read from a file.
	 ******************************************************************************/
	MapParams()
	{
		floatPropertyMap.emplace(std::pair<std::string, float&>("noiseSampleHeight", noiseSampleHeight));
		intPropertyMap.emplace(std::pair<std::string, int&>("scale", scale));
		floatPropertyMap.emplace(std::pair<std::string, float&>("baseVariance", baseVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("lieChangeRate", lieChangeRate));
		floatPropertyMap.emplace(std::pair<std::string, float&>("liePeak", liePeak));
		floatPropertyMap.emplace(std::pair<std::string, float&>("lieModif", lieModif));
		floatPropertyMap.emplace(std::pair<std::string, float&>("hillHeight", hillHeight));
		intPropertyMap.emplace(std::pair<std::string, int&>("hillRarity", hillRarity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("hillVariancePower", hillVariancePower));

		intPropertyMap.emplace(std::pair<std::string, int&>("divetRarity", divetRarity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("divetHillScalar", divetHillScalar));

		floatPropertyMap.emplace(std::pair<std::string, float&>("mountainHeight", mountainHeight));
		intPropertyMap.emplace(std::pair<std::string, int&>("mountainRarity", mountainRarity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("mountainThreshold", mountainThreshold));
		floatPropertyMap.emplace(std::pair<std::string, float&>("mountainConstantMultiplier", mountainConstantMultiplier));

		intPropertyMap.emplace(std::pair<std::string, int&>("generatedMapDensity", generatedMapDensity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilResistivityChangeRate", soilResistivityChangeRate));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilResistivityBase", soilResistivityBase));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilResistivityVariance", soilResistivityVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilSandContent", soilSandContent));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilClayContent", soilClayContent));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilSandVariance", soilSandVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilClayVariance", soilClayVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("soilFertility", soilFertility));

		floatPropertyMap.emplace(std::pair<std::string, float&>("rockRarity", rockRarity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("rockResistivityVariance", rockResistivityVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("rockVerticalScaling", rockVerticalScaling));
		floatPropertyMap.emplace(std::pair<std::string, float&>("rockResistivityBase", rockResistivityBase));
		floatPropertyMap.emplace(std::pair<std::string, float&>("rockThreshold", rockThreshold));

		floatPropertyMap.emplace(std::pair<std::string, float&>("springThreshold", springThreshold));
		floatPropertyMap.emplace(std::pair<std::string, float&>("minimumSpringHeight", minimumSpringHeight));
		intPropertyMap.emplace(std::pair<std::string, int&>("springRarity", springRarity));

		floatPropertyMap.emplace(std::pair<std::string, float&>("cliffThreshold", cliffThreshold));

		floatPropertyMap.emplace(std::pair<std::string, float&>("treeParticleDeathThreshold", treeParticleDeathThreshold));
		floatPropertyMap.emplace(std::pair<std::string, float&>("treeSlopeThreshold", treeSlopeThreshold));
		intPropertyMap.emplace(std::pair<std::string, int&>("treeSpreadChance", treeSpreadChance));
		intPropertyMap.emplace(std::pair<std::string, int&>("treeSpreadRadius", treeSpreadRadius));
		intPropertyMap.emplace(std::pair<std::string, int&>("treeLongDistanceFertilizationCount", treeLongDistanceFertilizationCount));
		floatPropertyMap.emplace(std::pair<std::string, float&>("foliageOverpopulationThreshold", foliageOverpopulationThreshold));
		intPropertyMap.emplace(std::pair<std::string, int&>("treeRandomDeathChance", treeRandomDeathChance));
		intPropertyMap.emplace(std::pair<std::string, int&>("treeGenerationRarity", treeGenerationRarity));

		floatPropertyMap.emplace(std::pair<std::string, float&>("streamEvaporationRate", streamEvaporationRate));
		floatPropertyMap.emplace(std::pair<std::string, float&>("particleEvaporationRate", particleEvaporationRate));
		intPropertyMap.emplace(std::pair<std::string, int&>("dropWidth", dropWidth));
		floatPropertyMap.emplace(std::pair<std::string, float&>("seaLevel", seaLevel));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropDefaultVolume", dropDefaultVolume));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropMinimumVolume", dropMinimumVolume));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropSedimentSimulationMinimumVelocity", dropSedimentSimulationMinimumVelocity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropSedimentSimulationTerminationVelocity", dropSedimentSimulationTerminationVelocity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropSedimentDepositCap", dropSedimentDepositCap));
		floatPropertyMap.emplace(std::pair<std::string, float&>("dropContainedSedimentCap", dropContainedSedimentCap));
		floatPropertyMap.emplace(std::pair<std::string, float&>("particleTerminationProximity", particleTerminationProximity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("particleSwayMagnitude", particleSwayMagnitude));
		floatPropertyMap.emplace(std::pair<std::string, float&>("floodDefaultIncrease", floodDefaultIncrease));
		floatPropertyMap.emplace(std::pair<std::string, float&>("drainErosionAmount", drainErosionAmount));
		floatPropertyMap.emplace(std::pair<std::string, float&>("poolSedimentLossRate", poolSedimentLossRate));

		floatPropertyMap.emplace(std::pair<std::string, float&>("sandHeightVariance", sandHeightVariance));
		floatPropertyMap.emplace(std::pair<std::string, float&>("minimumSandHeight", minimumSandHeight));
		floatPropertyMap.emplace(std::pair<std::string, float&>("sandResistivity", sandResistivity));
		floatPropertyMap.emplace(std::pair<std::string, float&>("sandFertility", sandFertility));

		floatPropertyMap.emplace(std::pair<std::string, float&>("bedrockResisitivity", bedrockResisitivity));
	}

	std::map<std::string, float&> floatPropertyMap;
	std::map<std::string, int&> intPropertyMap;

	// NOTE: additional map parameters need to be added to the constructor, filling the property map!
	float noiseSampleHeight = 0.5f;
	int scale = 1;
	float baseVariance = 0.05f;
	float lieChangeRate = 3000.0f;
	float liePeak = 25.0f;
	float lieModif = -24.0f;
	float hillHeight = 80.0f;
	int hillRarity = 1000;
	float hillVariancePower = 0.5f;

	int divetRarity = 85;
	float divetHillScalar = 0.05f;

	float mountainHeight = 200.0f;
	int mountainRarity = 5000;
	float mountainThreshold = 0.85;
	float mountainConstantMultiplier = 6.6f;

	int generatedMapDensity = 20;
	float soilResistivityChangeRate = 2000.0f;
	float soilResistivityBase = 2.3f;
	float soilResistivityVariance = 0.3f;
	float soilSandContent = 0.25f;
	float soilClayContent = 0.22f;
	float soilSandVariance = 0.15f;
	float soilClayVariance = 0.07f;
	float soilFertility = 0.8f;

	float rockRarity = 200.0f;
	float rockResistivityVariance = 0.6f;
	float rockVerticalScaling = 5.0f;
	float rockResistivityBase = 3.8f;
	float rockThreshold = 0.6f;

	float springThreshold = 0.99f;
	float minimumSpringHeight = 50.0f;
	int springRarity = 200;

	float cliffThreshold = 0.0007f;

	float treeParticleDeathThreshold = 0.1f;
	float treeSlopeThreshold = 0.985f;
	int treeSpreadChance = 5;
	int treeSpreadRadius = 9;
	int treeLongDistanceFertilizationCount = 5;
	float foliageOverpopulationThreshold = 0.8f;
	int treeRandomDeathChance = 100000;
	int treeGenerationRarity = 2;

	float streamEvaporationRate = 0.95f;
	float particleEvaporationRate = 0.985f;
	int dropWidth = 2;
	float seaLevel = 0.0f;
	float dropDefaultVolume = 1.0f;
	float dropMinimumVolume = 0.001f;
	float dropSedimentSimulationMinimumVelocity = 1.0f;
	float dropSedimentSimulationTerminationVelocity = 0.075f;
	float dropSedimentDepositCap = 0.015f;
	float dropContainedSedimentCap = 10.0f;
	float particleTerminationProximity = 4.0f;
	float particleSwayMagnitude = 0.05f;
	float floodDefaultIncrease = 0.01f;
	float drainErosionAmount = 0.005f;
	float poolSedimentLossRate = 0.5f;

	float sandHeightVariance = 0.5f;
	float sandResistivity = 1.5f;
	float sandFertility = 0.05f;
	float minimumSandHeight = 0.2f;

	float bedrockResisitivity = 7.5f;
};

/***************************************************************************//**
 * The map class serves as the simulation access point- all calls to
 * simulate anything will go through here. It also houses all node
 * data for the program.
 ******************************************************************************/
class Map 
{
public:
	/***************************************************************************//**
	 * The map constructor fills all node data to generate terrain using a defined seed
	 * if no seed is given, it will generate one based on the current time.
	 @param width The width of the map
	 @param height The height of the map
	 @param params Defines for generation and simulation within the map
	 @param seed The seed to generate the map from 
	 ******************************************************************************/
	Map(int width, int height, MapParams params, unsigned int seed = 0);
	~Map();

	/***************************************************************************//**
	 * Assumes a heightmap has been generated, and populates the area underneath
	 * with soil data. Uses perlin noise for randomisation.
	 @param resistivityNoise noise for terrain resistivity
	 @param rockNoise noise for rock generation (at heigher values rocks will generate)
	 ******************************************************************************/
	void addRocksAndDirt(PerlinNoise* resistivityNoise, PerlinNoise* rockNoise);
	/***************************************************************************//**
	 * Calculates a perlin noise sample coordinate based on rarity of a feature,
	 * scale, and the current position
	 @param x The current X coordinate
	 @param y The current Y coordinate
	 @param rarity Rarity (taken from the map parameters)
	 ******************************************************************************/
	glm::vec2 calculateXYFromRarity(int x, int y, float rarity);
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	float getScale() { return m_params.scale; }
	float getMaxHeight() { return m_maxHeight; }
	int getAge() { return m_age; }
	Node* getNodeAt(int x, int y);
	float getDensityAt(int x, int y, float height);
	float getHeightAt(int x, int y);
	float getHeightAt(int index);
	float getDepthAt(int index);
	float getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity);
	float getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity);
	float getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity);
	/***************************************************************************//**
	 * Defines all commonly-seen soils for comparison to map data
	 ******************************************************************************/
	void defineSoils();
	/***************************************************************************//**
	 * Debug. Erodes the top marker of every node on the map
	 ******************************************************************************/
	void skimTop();
	/***************************************************************************//**
	 * Try to create a spring at a location
	 ******************************************************************************/
	void addSpring(int x, int y);
	void erodeAllByValue(float amount);
	/***************************************************************************//**
	 * Generates a string of stats to output to the console. Soil types, height,
	 * colour, normal, etc
	 @param pos The position to sample
	 ******************************************************************************/
	std::string stats(glm::vec2 pos);
	/***************************************************************************//**
	 * Returns the string name of the soil type at the given node, and the certainty.
	 @param pos The position to sample
	 ******************************************************************************/
	std::string getSoilType(glm::vec2 pos);
	/***************************************************************************//**
	 * Returns the a general overview of the soil percentages throughout the map
	 * formatted as a string for console output.
	 ******************************************************************************/
	std::string getMapGeneralSoilType();
	/***************************************************************************//**
	 * Returns the index of the soil type at the given node.
	 @param nodeData The node to sample
	 @param bestCertainty The certainty of the outcome
	 ******************************************************************************/
	int getSoilTypeBestMatching(NodeMarker* nodeData, float& bestCertainty);
	bool trySpawnTree(glm::vec2 pos);

	// Hydrology Functions
	void erode(int cycles);
	void grow();

	glm::vec3 normal(int index)
	{
		glm::vec2 pos = glm::vec2(index % m_width, index / m_width);
		float left = getNodeAt(pos.x + 1, pos.y)->waterHeight(getNodeAt(pos.x + 1, pos.y)->topHeight());
		float right = getNodeAt(pos.x - 1, pos.y)->waterHeight(getNodeAt(pos.x - 1, pos.y)->topHeight());
		float up = getNodeAt(pos.x, pos.y + 1)->waterHeight(getNodeAt(pos.x, pos.y + 1)->topHeight());
		float down = getNodeAt(pos.x, pos.y - 1)->waterHeight(getNodeAt(pos.x, pos.y - 1)->topHeight());
		return glm::normalize(glm::vec3(2 * (right - left), 2 * (down - up), -4));
	}
	

protected:
	Node* m_nodes;
	int m_width;
	int m_height;
	int m_age;
	float m_maxHeight;
	std::vector<glm::vec2> m_springs;
	std::vector<SoilDefinition> m_soilDefinitions;
	MapParams m_params;
};