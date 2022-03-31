#include "Map.h"

#include <glm.hpp>
#include <ext.hpp>
#include <iostream>

#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"
#include "Vegetation.h"
#include "Water.h"

Map::Map(int width, int height, MapParams params, unsigned int seed)
{
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_maxHeight = 0.0f;
	m_scale = params.scale;

	if(seed == 0)
		srand(time(NULL));
	else
		srand(seed);

	int generatedSeed = rand() % 99999;
	PerlinNoise baseVarianceNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise hillNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise divetNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise mountainNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise lieNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise rockNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise sandNoise(generatedSeed);

	// Ensure our values are valid-  rarity must be a multiple of scale
	params.hillRarity -= (params.hillRarity % m_scale);
	params.mountainRarity -= (params.mountainRarity % m_scale);
	params.divetRarity -= (params.divetRarity % m_scale);
	const glm::vec3 colorVary = glm::vec3((float)(rand() % 100) / 100.0f * 0.45f, 0.9f + (float)(rand() % 100) / 100.0f * 0.1f, (float)(rand() % 100) / 100.0f * 0.45f);
	float completion = 0.0f;

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			const float val = baseVarianceNoise.noise(x, y, 0.5f) * params.baseVariance * 10;
			const float base = (lieNoise.noise(x/(params.lieChangeRate / m_scale), y/(params.lieChangeRate / m_scale), 0.5f) * params.liePeak / m_scale) + (params.lieModif / m_scale);
			const float hill = getHillValue(&hillNoise, x, y, params.hillHeight, params.hillRarity);
			const float div = getDivetValue(&divetNoise, x, y, params.hillHeight / 20.0f, params.divetRarity);
			const float mount = getMountainValue(&mountainNoise, x, y, params.mountainHeight, params.mountainRarity);
			const float total = (base + val + hill + mount + div);

			const float sandThreshold = sandNoise.noise(x, y, 0.5f) * 0.5f;

			if (total < sandThreshold)
			{
				const float mountSteep = getMountainSteepness(&mountainNoise, x, y, 1.0f, params.mountainRarity);
				const float hillSteep = getHillSteepness(&hillNoise, x, y, 1.0f, params.hillRarity);
				float currThreshold = params.cliffThreshold + (0.0001f * (mountainNoise.noise(x, y, 5.0f) - 0.5f));
				if (mountSteep > currThreshold || hillSteep > currThreshold)
				{
					float modif = -(pow(1.0f + (glm::max(0.0f, mountSteep - currThreshold) + glm::max(0.0f, hillSteep - currThreshold)), 2)) * 10.0f;
					m_nodes[y * width + x].addMarker(glm::max(-1.9f, total + modif), 3.5f, true, glm::vec3(0.8f, 0.8f, 0.8f), m_maxHeight);
				}
				else 
				{
					// sand (1.8g/cm3)
					// this is slightly hack-y. We know this is going to be low, so we set the color to >1.0f so it'll render brighter.
					m_nodes[y * width + x].addMarker(glm::max(-1.9f, total), 1.8f, false, glm::vec3(3.0f, 3.0f, 0.8f), m_maxHeight);
				}
			}
			else
			{
				// topsoil (2.3g/cm3)
				m_nodes[y * width + x].addMarker(glm::max(-1.9f, total), 2.3f, false, colorVary, m_maxHeight);
				m_nodes[y * width + x].top()->foliage = 1.0f;
			}
			// bedrock (7.0g/cm3)
			m_nodes[y * width + x].addMarker(-2.0f, 7.5f, true, glm::vec3(0.1f), m_maxHeight);
			m_nodes[y * width + x].addWaterToLevel(0.0f);
		}
		float prevCompletion = completion;
		completion = (x / (float)width) * 100.0f;
		if((int)completion % 10 < (int) prevCompletion % 10)
			std::cout << "Populating World Nodes: " << completion << "%" <<std::endl;
	}

	std::cout << "Populating World Nodes: 100%" << std::endl;
	addRocksAndDirt(params.rockVerticalScaling, params.rockDensityVariance, params.densityVariance, params.densityChangeRate, params.rockRarity);
}

void Map::addRocksAndDirt(float rockVerticalScaling, float rockDensityVariance, float densityVariance, float densityChangeRate, float rockRarity)
{
	float seed = rand() % 99999;
	PerlinNoise densityNoise(seed); 
	seed = rand() % 99999;
	PerlinNoise rockNoise(seed);
	float completion = 0.0f;

	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			bool isRock = false;
			float maxHeightScaled = getHeightAt(x, y) / m_maxHeight;
			for (float densHeight = 0.0f; densHeight < maxHeightScaled; densHeight += 0.05f)
			{
				const float scaledDensHeight = densHeight * rockVerticalScaling;
				if (!isRock)
				{
					// soil density (2.5-2.8g/cm3)
					float noise = densityNoise.noise(x / (densityChangeRate / m_scale), y / (densityChangeRate / m_scale), scaledDensHeight);
					float density = 2.5f + noise * densityVariance;
					glm::vec3 col = glm::vec3(0.2f + noise * 0.4f, 0.3f, 0.0f);
					m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, density, false, col, m_maxHeight);

					// rock density (3.8-4.2g/cm3)
					float currVal = rockNoise.noise(x / (rockRarity / m_scale), y / (rockRarity / m_scale), scaledDensHeight);
					if (currVal > 0.6f)
					{
						float density = 3.8f + (currVal - 0.6f) * rockDensityVariance;
						m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, density, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, m_maxHeight);
						isRock = true;
					}
				}
				else
				{
					float currVal = rockNoise.noise(x / (rockRarity / m_scale), y / (rockRarity / m_scale), scaledDensHeight);
					if (currVal < 0.6f)
					{
						m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, 3.2f, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, m_maxHeight);
						isRock = false;
					}
				}
			}
		}

		float prevCompletion = completion;
		completion = (x / (float)m_width) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
			std::cout << "Placing Rocks and Dirt: " << completion << "%" << std::endl;
	}
	std::cout << "Placing Rocks and Dirt: 100%" << std::endl;
}

float Map::getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float hillVal = (noise->noise(XY.x, XY.y, 0.5f) - 0.1f) * glm::pow(noise->noise(XY.x, XY.y, 1.0f), 0.5f);
	return hillVal * hillHeight / m_scale;
}

float Map::getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float divetVal = -(float)noise->noise(XY.x, XY.y, 0.5f);
	return divetVal * divetHeight / m_scale;
}

float Map::getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float mountainVal = 0.0f;
	bool mountain = noise->noise(XY.x, XY.y, 0.5f) > 0.85;
	if (mountain)
	{
		mountainVal += pow((noise->noise(XY.x, XY.y, 0.5f) - 0.85) * sqrt(mountainHeight) * 6.6f / m_scale, 2);
	}
	return mountainVal;
}

float Map::getMountainSteepness(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity)
{
	float XY = getMountainValue(noise, x, y, mountainHeight, rarity);
	float XYup = getMountainValue(noise, x, y - 1, mountainHeight, rarity);
	float XYdown = getMountainValue(noise, x, y + 1, mountainHeight, rarity);
	float XYleft = getMountainValue(noise, x - 1, y, mountainHeight, rarity);
	float XYright = getMountainValue(noise, x + 1, y, mountainHeight, rarity);

	float steepnessVar = 0;
	steepnessVar += abs(XY - XYup);
	steepnessVar += abs(XY - XYdown);
	steepnessVar += abs(XY - XYleft);
	steepnessVar += abs(XY - XYright);

	return (steepnessVar/4.0);
}

float Map::getHillSteepness(PerlinNoise* noise, int x, int y, float hillHeight, float rarity)
{
	float XY = getHillValue(noise, x, y, hillHeight, rarity);
	float XYup = getHillValue(noise, x, y - 1, hillHeight, rarity);
	float XYdown = getHillValue(noise, x, y + 1, hillHeight, rarity);
	float XYleft = getHillValue(noise, x - 1, y, hillHeight, rarity);
	float XYright = getHillValue(noise, x + 1, y, hillHeight, rarity);

	float steepnessVar = 0;
	steepnessVar += abs(XY - XYup);
	steepnessVar += abs(XY - XYdown);
	steepnessVar += abs(XY - XYleft);
	steepnessVar += abs(XY - XYright);

	return (steepnessVar / 4.0);
}

glm::vec2 Map::calculateXYFromRarity(int x, int y, float rarity)
{
	float scalar = (rarity / m_scale);
	int lowX = x / scalar;
	int lowY = y / scalar;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % (int)scalar;
	float yOffset = y % (int)scalar;
	float xValue = lowX + (xOffset / scalar) * (highX - lowX);
	float yValue = lowY + (yOffset / scalar) * (highY - lowY);
	return glm::vec2(xValue, yValue);
}

Node* Map::getNodeAt(int x, int y)
{
	if (y >= m_height)
	{
		return getNodeAt(x, fmin(y, m_height - 1));
	}
	if (x >= m_width)
	{
		return getNodeAt(fmin(x, m_width - 1), y);
	}
	if (x < 0)
	{
		return getNodeAt(0, y);
	}
	if (y < 0)
	{
		return getNodeAt(x, 0);
	}

	return &m_nodes[y * m_width + x];
}

float Map::getHeightAt(int x, int y)
{
	if (y >= m_height)
	{
		return getHeightAt(x, fmin(y, m_height - 1));
	}
	if (x >= m_width)
	{
		return getHeightAt(fmin(x, m_width - 1), y);
	}
	if (x < 0)
	{
		return getHeightAt(0, y);
	}
	if (y < 0)
	{
		return getHeightAt(x, 0);
	}

	return m_nodes[y * m_width + x].top()->height;
}

float Map::getDensityAt(int x, int y, float height)
{
	return getNodeAt(x, y)->getDensityAtHeight(height);
}

// Debug function. Removes top layer of every node, to test density values etc. without erosion
void Map::skimTop()
{
	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			getNodeAt(x, y)->skim();
		}
	}
}

// Debug function. Erodes everything, to test erosion.
void Map::erodeAllByValue(float amount)
{
	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			getNodeAt(x, y)->erodeByValue(amount);
			getNodeAt(x, y)->addWaterToLevel(0.0f);
		}
	}
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
}
/*
===================================================
		  HYDRAULIC EROSION FUNCTIONS
===================================================

void Map::erode(int cycles) {

	//Track the Movement of all Particles
	float track[m_width * m_height] = { 0.0f };

	//Do a series of iterations!
	for (int i = 0; i < cycles; i++) {

		//Spawn New Particle
		glm::vec2 newpos = glm::vec2(rand() % (int)m_width, rand() % (int)m_height);
		Drop drop(newpos);

		while (true) {

			while (drop.descend(normal((int)drop.pos.x * m_height + (int)drop.pos.y), heightmap, waterpath, waterpool, track, plantdensity, dim, SCALE));
			if (!drop.flood(heightmap, waterpool, dim))
				break;

		}

	}

	//Update Path
	float lrate = 0.01;
	for (int i = 0; i < m_width * m_height; i++)
		waterpath[i] = (1.0 - lrate) * waterpath[i] + lrate * 50.0f * track[i] / (1.0f + 50.0f * track[i]);

}

bool Map::grow() {

	//Random Position
	{
		int i = rand() % (m_width * m_height);
		glm::vec3 n = normal(i);

		if (waterpool[i] == 0.0 &&
			waterpath[i] < 0.2 &&
			n.y > 0.8) {

			Plant ntree(i, dim);
			ntree.root(plantdensity, dim, 1.0);
			trees.push_back(ntree);
		}
	}

	//Loop over all Trees
	for (int i = 0; i < trees.size(); i++) {

		//Grow the Tree
		trees[i].grow();

		//Spawn a new Tree!
		if (rand() % 50 == 0) {
			//Find New Position
			glm::vec2 npos = trees[i].pos + glm::vec2(rand() % 9 - 4, rand() % 9 - 4);

			//Check for Out-Of-Bounds
			if (npos.x >= 0 && npos.x < dim.x &&
				npos.y >= 0 && npos.y < dim.y) {

				Plant ntree(npos, dim);
				glm::vec3 n = normal(ntree.index);

				if (waterpool[ntree.index] == 0.0 &&
					waterpath[ntree.index] < 0.2 &&
					n.y > 0.8 &&
					(float)(rand() % 1000) / 1000.0 > plantdensity[ntree.index]) {
					ntree.root(plantdensity, dim, 1.0);
					trees.push_back(ntree);
				}
			}
		}

		//If the tree is in a pool or in a stream, kill it
		if (waterpool[trees[i].index] > 0.0 ||
			waterpath[trees[i].index] > 0.2 ||
			rand() % 1000 == 0) { //Random Death Chance
			trees[i].root(plantdensity, dim, -1.0);
			trees.erase(trees.begin() + i);
			i--;
		}
	}

	return true;

};
*/