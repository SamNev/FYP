#pragma once

#include <time.h>

#include "Node.h"
#include "SDL2/SDL.h"

class PerlinNoise;

struct MapParams
{
	float getRandomModif()
	{
		return (float)(((rand() % 1000) / 1000.0f) * 1.4f) + 0.3f;
	}

	void randomize()
	{
		srand(time(NULL));
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
};

class Map 
{
public:
	Map(int width, int height, MapParams params);
	~Map();

	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	float getScale() { return m_scale; }
	float getMaxHeight() { return m_maxHeight; }
	Node* getNodeAt(int x, int y);
	float getDensityAt(int x, int y, float height);
	float getHeightAt(int x, int y);
	float getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity);
	float getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity);
	float getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity);
	void skimTop();
protected:
	int m_scale = 10;
	Node* m_nodes;
	int m_width;
	int m_height;
	float m_maxHeight;
};