#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <vector>

struct WaterData
{
	float height;
};

struct NodeMarker
{
	float height;
	float density;
	bool hardStop;
	float foliage = 0.0f;
	glm::vec3 color;
};

class Node {
public:
	void addWater(float height);
	void addWaterToLevel(float height);
	void addMarker(float height, float density, bool hardStop, glm::vec3 color, float& maxHeight);
	void erodeByValue(float amount);
	float getDensityAtHeight(float height) const;
	glm::vec3 getColorAtHeight(float height) const;
	float topHeight() const;
	glm::vec3 topColor() const;
	NodeMarker* top();
	void skim();
	float waterHeight(float valIfNoWater) const;
	bool hasWater() const;
protected:
	std::vector<NodeMarker> m_nodeData;
	WaterData m_waterData;
};