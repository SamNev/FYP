#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <string>
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
	float fertility = 0.8f;
	float sandAmount = 0.3f;
	float clayAmount = 0.25f;
	glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);

	void mix(NodeMarker marker, float weight)
	{
		float invWeight = (1.0f - weight);
		height = height * invWeight + marker.height * weight;
		resistiveForce = resistiveForce * invWeight + marker.resistiveForce * weight;
		hardStop = false;
		color = color * invWeight + marker.color * weight;
		fertility = fertility * invWeight + marker.fertility * weight;
		sandAmount = sandAmount * invWeight + marker.sandAmount * weight;
		clayAmount = clayAmount * invWeight + marker.clayAmount * weight;
	}
};

struct SoilDefinition
{
	std::string name;
	glm::vec2 clayBounds;
	glm::vec2 sandBounds;
	glm::vec2 fertBounds;
	glm::vec2 resBounds;
	bool hardStop;

	SoilDefinition(std::string definitionName, glm::vec2 definitionClayBounds, glm::vec2 definitionSandBounds, glm::vec2 definitionFertBounds, glm::vec2 definitionResBounds, bool isHardStop = false)
	{
		name = definitionName;
		clayBounds = definitionClayBounds;
		sandBounds = definitionSandBounds;
		fertBounds = definitionFertBounds;
		resBounds = definitionResBounds;
		hardStop = isHardStop;
	}

	float percForBounds(float val, glm::vec2 bounds)
	{
		if (val >= bounds.x)
		{
			if (val <= bounds.y)
				return 25.0f;
			else
				return 125.0f * glm::max<float>(0.0f, (0.2f - abs(val - bounds.y)));
		}
		else
		{
			return 125.0f * glm::max<float>(0.0f, (0.2f - abs(bounds.x - val)));
		}
	}

	float getCertainty(NodeMarker* marker)
	{
		float cert = 0.0f;
		cert += percForBounds(marker->clayAmount, clayBounds);
		cert += percForBounds(marker->sandAmount, sandBounds);
		cert += percForBounds(marker->fertility, fertBounds);
		cert += percForBounds(marker->resistiveForce, resBounds);
		if (marker->hardStop != hardStop)
			cert /= 2.0f;

		return cert;
	}
};

class Node {
public:
	void addWater(float height);
	void addMarker(NodeMarker marker, float& maxHeight);
	void addMarker(float height, float resistiveForce, bool hardStop, glm::vec3 color, float fertility, float sandAmount, float clayAmount, float& maxHeight);
	void erodeByValue(float amount);
	float getResistiveForceAtHeight(float height) const;
	NodeMarker getDataAboveHeight(float height, bool ignoreRock = false) const;
	glm::vec3 getColorAtHeight(float height) const;
	float topHeight() const;
	glm::vec3 topColor() const;
	NodeMarker* top();
	void skim();
	float waterHeight() const;
	float waterHeight(float valIfNoWater) const;
	float waterHeightWithStreams(float valIfNoWater) const;
	float waterDepth() const;
	void setWaterHeight(float waterHeight);
	void setWaterDepth(float waterDepth);
	bool hasWater() const;
	void setHeight(float height, NodeMarker fillerValue, float& maxHeight);
	float getParticles() const;
	void setParticles(float particles);
	void setFoliageDensity(float density);
	float getFoliageDensity() const;
protected:
	std::vector<NodeMarker> m_nodeData;
	WaterData m_waterData;
	VegetationData m_vegetationData;
};