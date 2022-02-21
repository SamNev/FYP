#pragma once

#include "Node.h"
#include "SDL2/SDL.h"

class PerlinNoise;

struct MapParams
{
	float scale = 10.0f;
	float baseVariance = 0.07f;
	float lieChangeRate = 10000.0f;
	float liePeak = 25.0f;
	float lieModif = -20.0f;
	float hillHeight = 100.0f;
	float hillRarity = 500.0f;
	float mountainHeight = 300.0f;
	float mountainRarity = 2000.0f;
};

class Map 
{
public:
	Map(int width, int height, MapParams params);
	~Map();

	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	float getScale() { return m_scale; }
	Node* getNodeAt(int x, int y);
	float getHeightAt(int x, int y);
	float getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity);
	float getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity);
protected:
	float m_scale = 10.0f;
	Node* m_nodes;
	int m_width;
	int m_height;
};