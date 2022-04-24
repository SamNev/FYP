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
	float height = -2.0f;
	float resistiveForce = 6.0f;
	bool hardStop = false;
	float foliage = 0.0f;
	glm::vec3 color = glm::vec3(1.0f, 0.0f, 1.0f);

	NodeMarker mix(NodeMarker marker, float weight)
	{
		NodeMarker newMarker;
		float invWeight = (1.0f - weight);
		newMarker.height = height * invWeight + marker.height * weight;
		newMarker.resistiveForce = resistiveForce * invWeight + marker.resistiveForce * weight;
		newMarker.hardStop = false;
		newMarker.foliage = foliage * invWeight + marker.foliage * weight;
		newMarker.color = color * invWeight + marker.color * weight;
		return newMarker;
	}
};

class Node {
public:
	void addWater(float height);
	void addMarker(NodeMarker marker);
	void addMarker(float height, float resistiveForce, bool hardStop, glm::vec3 color, float& maxHeight);
	void erodeByValue(float amount);
	float getResistiveForceAtHeight(float height) const;
	NodeMarker getDataAboveHeight(float height) const;
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
	float getParticles() const;
	void setParticles(float particles);
	void setFoliageDensity(float density);
	float getFoliageDensity() const;
protected:
	std::vector<NodeMarker> m_nodeData;
	WaterData m_waterData;
	VegetationData m_vegetationData;
};