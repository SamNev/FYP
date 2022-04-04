#pragma once

#include <time.h>

#include "Node.h"
#include "Plant.h"
#include "SDL2/SDL.h"

class PerlinNoise;

struct MapParams
{
	float getRandomModif()
	{
		return (float)(((rand() % 1000) / 1000.0f) * 1.4f) + 0.3f;
	}

	void randomize(unsigned int seed = 0)
	{
		if (seed == 0)
			srand(time(NULL));
		else
			srand(seed);

		scale = 3;
		baseVariance = 0.05f;
		lieChangeRate = 3000.0f * getRandomModif();
		liePeak = 25.0f * getRandomModif();
		lieModif = -24.0f * getRandomModif();
		hillHeight = 80.0f * getRandomModif();
		hillRarity = (int)(1000.0f * getRandomModif());
		mountainHeight = 2000.0f * getRandomModif();
		mountainRarity = (int)(5000.0f * getRandomModif());
		divetRarity = (int)(85.0f * getRandomModif());
		rockVerticalScaling = 5.0f * getRandomModif();
	}

	void tweak(float amount)
	{
		liePeak *= (1.0 + amount);
		lieModif *= (1.0 + amount);
		hillHeight *= (1.0 + amount);
		mountainHeight *= (1.0 + amount);
	}

	int scale = 10;
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
};

class Map 
{
public:
	Map(int width, int height, MapParams params, unsigned int seed = 0);
	~Map();

	void addRocksAndDirt(float rockVerticalScaling, float rockDensityVariance, float densityVariance, float densityChangeRate, float rockRarity);
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
	float getMountainSteepness(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity);
	float getHillSteepness(PerlinNoise* noise, int x, int y, float hillHeight, float rarity);
	void skimTop();
	void erodeAllByValue(float amount);

	//Simple Hydrology Functions
	void erode(int cycles);
	bool grow();

	glm::vec3 normal(int index){

	//Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
	glm::vec3 n = glm::cross(glm::vec3(0.0, m_scale*(getHeightAt(index+1)-getHeightAt(index)+getDepthAt(index+1)-getDepthAt(index)), 1.0), glm::vec3(1.0, m_scale*(getHeightAt(index+m_height)+getDepthAt(index+m_height)-getHeightAt(index)-getDepthAt(index)), 0.0));
	n += glm::cross(glm::vec3(0.0, m_scale*(getHeightAt(index-1)- getHeightAt(index) + getDepthAt(index-1)-getDepthAt(index)), -1.0), glm::vec3(-1.0, m_scale*(getHeightAt(index-m_height)-getHeightAt(index)+getDepthAt(index-m_height)-getDepthAt(index)), 0.0));

	//Two Alternative Planes (+X -> -Y) (-X -> +Y)
	n += glm::cross(glm::vec3(1.0, m_scale*(getHeightAt(index+m_height)-getHeightAt(index)+getDepthAt(index+m_height)-getDepthAt(index)), 0.0), glm::vec3(0.0, m_scale*(getHeightAt(index-1)-getHeightAt(index)+getDepthAt(index-1)-getDepthAt(index)), -1.0));
	n += glm::cross(glm::vec3(-1.0, m_scale*(getHeightAt(index+m_height)-getHeightAt(index)+getDepthAt(index+m_height)-getDepthAt(index)), 0.0), glm::vec3(0.0, m_scale*(getHeightAt(index+1)-getHeightAt(index)+getDepthAt(index+1)-getDepthAt(index)), 1.0));

	return glm::normalize(n);
  }
	

protected:
	int m_scale = 10;
	Node* m_nodes;
	int m_width;
	int m_height;
	float m_maxHeight;
	std::vector<Plant> m_trees;
};