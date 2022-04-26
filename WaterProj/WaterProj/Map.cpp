#include "Map.h"

#include <glm.hpp>
#include <ext.hpp>
#include <iostream>
#include <sstream>

#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"
#include "Plant.h"
#include "Drop.h"

Map::Map(int width, int height, MapParams params, unsigned int seed)
{
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_maxHeight = 0.0f;
	m_scale = params.scale;
	m_params = params;

	// seed based on time or whatever was given
	if(seed == 0)
		srand(time(NULL));
	else
		srand(seed);

	// a selection of varying perlin noise is needed to generate complex terrain
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
	PerlinNoise densityNoise(generatedSeed);
	generatedSeed = rand() % 99999;
	PerlinNoise sandNoise(generatedSeed);

	// Ensure our values are valid-  rarity must be a multiple of scale in these regards or we hit rounding errors.
	params.hillRarity -= (params.hillRarity % m_scale);
	params.mountainRarity -= (params.mountainRarity % m_scale);
	params.divetRarity -= (params.divetRarity % m_scale);

	float completion = 0.0f;

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			float val = baseVarianceNoise.noise(x, y, 0.5f) * params.baseVariance * 10;
			val = 0;
			const float base = (lieNoise.noise(x/(params.lieChangeRate / m_scale), y/(params.lieChangeRate / m_scale), 0.5f) * params.liePeak / m_scale) + (params.lieModif / m_scale);
			const float hill = getHillValue(&hillNoise, x, y, params.hillHeight, params.hillRarity);
			const float div = getDivetValue(&divetNoise, x, y, params.hillHeight / 20.0f, params.divetRarity);
			const float mount = getMountainValue(&mountainNoise, x, y, params.mountainHeight, params.mountainRarity);
			float total = (base + val + hill + mount + div);

#ifdef FLOODTESTMAP
			total = (abs(x-500) + abs(y-500))/20.0f;
#endif

			const float sandThreshold = sandNoise.noise(x, y, 0.5f) * 0.5f;

			if (total < sandThreshold)
			{
				const float mountSteep = getMountainSteepness(&mountainNoise, x, y, 1.0f, params.mountainRarity);
				const float hillSteep = getHillSteepness(&hillNoise, x, y, 1.0f, params.hillRarity);
				float currThreshold = params.cliffThreshold + (0.0001f * (mountainNoise.noise(x, y, 5.0f) - 0.5f));
				/*if (mountSteep > currThreshold || hillSteep > currThreshold)
				{
					float modif = -(pow(1.0f + (glm::max(0.0f, mountSteep - currThreshold) + glm::max(0.0f, hillSteep - currThreshold)), 2)) * 10.0f;
					m_nodes[y * width + x].addMarker(glm::max(-1.9f, total + modif), 3.5f, true, glm::vec3(0.8f, 0.8f, 0.8f), m_maxHeight);
				}
				else */
				{
					// sand (1.8g/cm3)
					// this is slightly hack-y. We know this is going to be low, so we set the color to >1.0f so it'll render brighter.
					m_nodes[y * width + x].addMarker(glm::max(-1.9f, total), 1.8f, false, glm::vec3(3.0f, 3.0f, 0.8f), 0.1f, m_maxHeight);
				}
			}
			else
			{
				// topsoil (2.3g/cm3)
				float topNoise = densityNoise.noise(x / (params.densityChangeRate / m_scale), y / (params.densityChangeRate / m_scale), glm::max(-1.9f, total));
				m_nodes[y * width + x].addMarker(glm::max(-1.9f, total), 2.3f, false, glm::vec3(0.2f + topNoise * 0.4f, 0.3f, 0.0f), 1.0f, m_maxHeight);
			}
			// bedrock (7.0g/cm3)
			m_nodes[y * width + x].addMarker(-2.0f, 7.5f, true, glm::vec3(0.1f), 0.0f, m_maxHeight);
			m_nodes[y * width + x].setWaterHeight(0.0f);
		}
		float prevCompletion = completion;
		completion = (x / (float)width) * 100.0f;
		if((int)completion % 10 < (int) prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Generating World Nodes: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;

	addRocksAndDirt(params.rockVerticalScaling, params.rockDensityVariance, params.densityVariance, params.densityChangeRate, params.rockRarity, &densityNoise, &rockNoise);
}

void Map::addRocksAndDirt(float rockVerticalScaling, float rockDensityVariance, float densityVariance, float densityChangeRate, float rockRarity, PerlinNoise* densityNoise, PerlinNoise* rockNoise)
{
	float completion = 0.0f;

	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			bool isRock = false;
			float maxHeightScaled = getHeightAt(x, y) / m_maxHeight;

			if (rand() % 2 == 0)
				trySpawnTree(glm::vec2(x, y));

			// peak heights can be springs, spawning water constantly
			if (maxHeightScaled >= m_params.springThreshold && rand() % m_params.springRarity == 0)
				addSpring(x, y);

			for (float densHeight = 0.0f; densHeight < maxHeightScaled; densHeight += 0.05f)
			{
				const float scaledDensHeight = densHeight * rockVerticalScaling;
				if (!isRock)
				{
					// soil resistiveForce (2.5-2.8g/cm3)
					float noise = densityNoise->noise(x / (densityChangeRate / m_scale), y / (densityChangeRate / m_scale), scaledDensHeight);
					float density = 2.5f + noise * densityVariance;
					glm::vec3 col = glm::vec3(0.2f + noise * 0.2f, 0.3f, 0.0f);
					m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, density, false, col, 1.0f, m_maxHeight);

					// rock resistiveForce (3.8-4.2g/cm3)
					float currVal = rockNoise->noise(x / (rockRarity / m_scale), y / (rockRarity / m_scale), scaledDensHeight);
					if (currVal > 0.6f)
					{
						float density = 3.8f + (currVal - 0.6f) * rockDensityVariance;
						m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, density, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, 0.0f, m_maxHeight);
						isRock = true;
					}
				}
				else
				{
					float currVal = rockNoise->noise(x / (rockRarity / m_scale), y / (rockRarity / m_scale), scaledDensHeight);
					if (currVal < 0.6f)
					{
						m_nodes[y * m_width + x].addMarker(densHeight * m_maxHeight, 3.2f, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, 0.0f, m_maxHeight);
						isRock = false;
					}
				}
			}
		}

		float prevCompletion = completion;
		completion = (x / (float)m_width) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Placing Rocks, Trees, and Dirt: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;
}

void Map::addSpring(int x, int y)
{
	m_springs.push_back(glm::vec2(x, y));
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

	return m_nodes[y * m_width + x].topHeight();
}

float Map::getHeightAt(int index)
{
	if (index >= (m_height * m_width))
	{
		return getHeightAt((m_height * m_width) - 1);
	}
	if (index < 0)
	{
		return getHeightAt(0);
	}

	return m_nodes[index].topHeight();
}

float Map::getDepthAt(int index)
{
	if (index >= (m_height * m_width))
	{
		return getDepthAt((m_height * m_width) - 1);
	}
	if (index < 0)
	{
		return getDepthAt(0);
	}

	return m_nodes[index].waterHeight(0.0f);
}

float Map::getDensityAt(int x, int y, float height)
{
	return getNodeAt(x, y)->getResistiveForceAtHeight(height);
}

// Debug function. Removes top layer of every node, to test resistiveForce values etc. without erosion
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
			getNodeAt(x, y)->setWaterHeight(0.0f);
		}
	}
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
}

std::string Map::stats(glm::vec2 pos)
{
	std::ostringstream oss;
	glm::vec3 norm = normal(pos.y * m_width + pos.x);
	glm::vec3 col = getNodeAt(pos.x, pos.y)->topColor();
	glm::vec3 col2 = getNodeAt(pos.x, pos.y)->top()->color;
	oss << "Node data at pos " << pos.x << ", " << pos.y << ": Land height = " << getNodeAt(pos.x, pos.y)->topHeight() << " Pool = " << getNodeAt(pos.x, pos.y)->waterDepth() << " Stream = " << getNodeAt(pos.x, pos.y)->getParticles() << " Foliage = " << getNodeAt(pos.x, pos.y)->getFoliageDensity() << " Normal is " << norm.x << ", " << norm.y << ", " << norm.z << " Color is " << col.x << ", " << col.y << ", " << col.z << " node color is " << col2.x << ", " << col2.y << ", " << col2.z << std::endl;
	return oss.str();
}

/*
===================================================
		  HYDRAULIC EROSION FUNCTIONS
===================================================
*/

void Map::erode(int cycles) {

	// all particle movement
	bool* track = new bool[m_width * m_height];
	for (int i = 0; i < m_width * m_height; i++)
		track[i] = false;

	glm::vec2 dim = glm::vec2(m_width, m_height);
	int springIndex = 0;

	float completion = 0.0f;

	for (int i = 0; i < cycles; i++)
	{
		// spawn particle
		glm::vec2 newpos = glm::vec2(rand() % m_width, rand() % m_height);

		// spawn at spring if possible
		if (springIndex < m_springs.size())
		{
			newpos = m_springs.at(springIndex);
			springIndex++;
		}

		Drop drop(newpos);

		int spill = 500;

		while (drop.getVolume() > drop.getMinVolume() && spill != 0) {

			if (!drop.descend(normal((int)drop.getPosition().y * m_width + (int)drop.getPosition().x), m_nodes, track, dim) && drop.getVolume() > drop.getMinVolume())
			{
				if (!drop.flood(m_nodes, dim))
					break;
			}

			spill--;
		}

		if (spill == 0)
			drop.flood(m_nodes, dim);

		float prevCompletion = completion;
		completion = (i / (float)cycles) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Running water simulation: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;

	for (int i = 0; i < m_width * m_height; i++)
	{
		if (track[i])
		{
			m_nodes[i].setParticles(m_nodes[i].getParticles() + 1.0f);
			float h = m_nodes[i].topHeight() + 1.0f;

			for (int yOffset = 0; yOffset < 9; yOffset++)
			{
				for (int xOffset = 0; xOffset < 9; xOffset++)
				{
					getNodeAt(i % m_width - 4, i / m_width - 4)->setParticles(glm::min(1.0f, glm::max(0.0f, h - getNodeAt(i % m_width, i / m_width)->topHeight())));
				}
			}
		}
		else
			m_nodes[i].setParticles(glm::max(0.0f, m_nodes->getParticles() - 0.1f));
	}

	delete[m_width * m_height] track;
}

void Map::grow()
{
	// spawn a tree randomly on the map (long-distance fertilization)
	int newTreePos = rand() % (m_width * m_height);
	glm::vec3 n = normal(newTreePos);
	int completion = 0;

	trySpawnTree(glm::vec2(newTreePos % m_width, newTreePos / m_width));

	for (int i = 0; i < m_trees.size(); i++) 
	{
		m_trees[i].grow();

		// tree spawns a new tree
		if (rand() % m_params.treeSpreadChance == 0)
		{
			glm::vec2 newPlantPos = m_trees[i].getPosition() + glm::vec2(rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius/2), rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius / 2));
			trySpawnTree(newPlantPos);
		}

		// trees die in water & sometimes die randomly
		if (m_nodes[m_trees[i].getIndex()].waterDepth() > 0.0 || m_nodes[m_trees[i].getIndex()].getParticles() > m_params.treeParticleDeathThreshold || rand() % m_params.treeRandomDeathChance == 0) 
		{ 
			m_trees[i].root(m_nodes, glm::vec2(m_width, m_height), -1.0f);
			m_trees.erase(m_trees.begin() + i);
		}
		float prevCompletion = completion;
		completion = (i / (float)m_trees.size()) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Running foliage simulation: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;
};

bool Map::trySpawnTree(glm::vec2 pos)
{
	if (pos.x < 0 || pos.x >= m_width || pos.y < 0 || pos.y >= m_height)
		return false;

	Plant newTree(pos, glm::vec2(m_width, m_height));
	if (m_nodes[newTree.getIndex()].top()->hardStop)
		return false;

	if (m_nodes[newTree.getIndex()].getFoliageDensity() >= 0.8f)
		return false;

	if (m_nodes[newTree.getIndex()].hasWater())
		return false;

	if(m_nodes[newTree.getIndex()].getParticles() > 0.2f)
		return false;

	glm::vec3 norm = normal(newTree.getIndex());
	if (abs(norm.z) < m_params.treeSlopeThreshold)
		return false;

	newTree.root(m_nodes, glm::vec2(m_width, m_height), 0.5f);
	m_trees.push_back(newTree);
	return true;
}