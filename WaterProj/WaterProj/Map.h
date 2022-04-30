#pragma once

#include <string>
#include <time.h>

#include "Node.h"
#include "Plant.h"
#include "SDL2/SDL.h"

//#define FLOODTESTMAP

class PerlinNoise;

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

struct MapParams
{
	float getRandomModif()
	{
		return (float)(((rand() % 1000) / 1000.0f) * 1.4f) + 0.3f;
	}

	void randomize(unsigned int seed = 0)
	{
		if (seed == 0)
			srand((unsigned int)time(NULL));
		else
			srand(seed);

		baseVariance = 0.05f;
		lieChangeRate = 3000.0f * getRandomModif();
		liePeak = 25.0f * getRandomModif();
		lieModif = -21.0f * getRandomModif();
		hillHeight = 80.0f * getRandomModif();
		hillRarity = (int)(1000.0f * getRandomModif());
		mountainHeight = 2000.0f * getRandomModif();
		mountainRarity = (int)(5000.0f * getRandomModif());
		divetRarity = (int)(85.0f * getRandomModif());
		rockVerticalScaling = 5.0f * getRandomModif();
	}

	void tweak(float amount)
	{
		liePeak *= (1.0f + amount);
		lieModif *= (1.0f + amount);
		hillHeight *= (1.0f + amount);
		mountainHeight *= (1.0f + amount);
	}

	int scale = 1;
	float baseVariance = 0.05f;
	float lieChangeRate = 3000.0f;
	float liePeak = 25.0f;
	float lieModif = -24.0f;
	float hillHeight = 80.0f;
	int hillRarity = 1000;
	int divetRarity = 85;
	float mountainHeight = 200.0f;
	int mountainRarity = 5000;
	float densityChangeRate = 2000.0f;
	float densityVariance = 0.3f;
	float rockRarity = 200.0f;
	float rockDensityVariance = 0.6f;
	float rockVerticalScaling = 5.0f;
	float cliffThreshold = 0.0007f;
	float springThreshold = 0.99f;
	int springRarity = 200;
	float treeParticleDeathThreshold = 0.1f;
	float treeSlopeThreshold = 0.985f;
	int treeSpreadChance = 5;
	int treeSpreadRadius = 9;
	float foliageOverpopulationThreshold = 0.8f;
	float waterEvaporationRate = 0.1f;
	int treeRandomDeathChance = 100000;
	int dropWidth = 4;

};

class Map 
{
public:
	Map(int width, int height, MapParams params, unsigned int seed = 0);
	~Map();

	void addRocksAndDirt(float rockVerticalScaling, float rockResistivityVariance, float densityVariance, float resistivityChangeRate, float rockRarity, PerlinNoise* resistivityNoise, PerlinNoise* rockNoise);
	glm::vec2 calculateXYFromRarity(int x, int y, float rarity);
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	float getScale() { return m_scale; }
	float getMaxHeight() { return m_maxHeight; }
	Node* getNodeAt(int x, int y);
	float getDensityAt(int x, int y, float height);
	float getHeightAt(int x, int y);
	float getHeightAt(int index);
	float getDepthAt(int index);
	float getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity);
	float getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity);
	float getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity);
	void defineSoils();
	void skimTop();
	void addSpring(int x, int y);
	void erodeAllByValue(float amount);
	std::string stats(glm::vec2 pos);
	std::string getSoilType(glm::vec2 pos);
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
	int m_scale = 10;
	Node* m_nodes;
	int m_width;
	int m_height;
	float m_maxHeight;
	std::vector<glm::vec2> m_springs;
	std::vector<SoilDefinition> m_soilDefinitions;
	MapParams m_params;
};