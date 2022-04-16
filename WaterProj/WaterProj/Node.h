#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <vector>

struct WaterData
{
	float height;
	float particles;
};

struct VegetationData
{
	float density;
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
	void addMarker(NodeMarker marker);
	void addMarker(float height, float density, bool hardStop, glm::vec3 color, float& maxHeight);
	void erodeByValue(float amount);
	float getDensityAtHeight(float height) const;
	glm::vec3 getColorAtHeight(float height) const;
	float topHeight() const;
	glm::vec3 topColor() const;
	NodeMarker* top();
	void skim();
	float waterHeight(float valIfNoWater) const;
	float waterHeightWithStreams(float valIfNoWater) const;
	float waterDepth() const;
	void setWaterHeight(float waterHeight);
	void setWaterDepth(float waterDepth);
	bool hasWater() const;
	void setHeight(float height, NodeMarker fillerValue);
	float getParticles();
	void setParticles(float particles);
	void setFoliageDensity(float density);
	float getFoliageDensity() const;
protected:
	std::vector<NodeMarker> m_nodeData;
	WaterData m_waterData;
	VegetationData m_vegetationData;
};