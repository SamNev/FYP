#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <string>
#include <vector>

/***************************************************************************//**
 * Data attaining to the amount of water on a node
 ******************************************************************************/
struct WaterData
{
	float height;
	float particles;
};

/***************************************************************************//**
 * Data attaining to the plant coverage and fertility of a node
 ******************************************************************************/
struct VegetationData
{
	float density;
	float waterSupply;
};

/***************************************************************************//**
 * A point defined at a given height within a node, to represent a marker on a vertical soil map.
 ******************************************************************************/
struct NodeMarker
{
	float height = -2.0f;
	float resistiveForce = 6.0f;
	bool hardStop = false;
	float fertility = 0.8f;
	float sandAmount = 0.3f;
	float clayAmount = 0.25f;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	/***************************************************************************//**
	 * Mix the values within this and another marker. 
	 @param marker The marker to mix with
	 @param weight The weight (0-1) of the other marker's value compared to this marker.
	 ******************************************************************************/
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

/***************************************************************************//**
 * Defines a soil and the boundaries of its properties
 ******************************************************************************/
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

	/***************************************************************************//**
	 * Works out the certainty that the given marker is of this soil type by
	 * comparing the bounds with marker values.
	 @param marker The marker to compare against.
	 ******************************************************************************/
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
	/***************************************************************************//**
	 * Erodes the terrain by a height value
	 @param amount The amount to erode by
	 ******************************************************************************/
	void erodeByValue(float amount);
	float getResistiveForceAtHeight(float height) const;
	/***************************************************************************//**
	 * Returns a NodeMarker containing all soil data above a given height mixed together.
	 @param height The height to check above
	 @param ignoreRock Whether rocks should be considered or not.
	 ******************************************************************************/
	NodeMarker getDataAboveHeight(float height, bool ignoreRock = false) const;
	glm::vec3 getColorAtHeight(float height) const;
	/***************************************************************************//**
	 * Returns the heighest height of the node.
	 ******************************************************************************/
	float topHeight() const;
	glm::vec3 topColor() const;
	/***************************************************************************//**
	 * Returns the topmost marker of the node
	 ******************************************************************************/
	NodeMarker* top();
	/***************************************************************************//**
	 * Debug- remove the top layer of the node.
	 ******************************************************************************/
	void skim();
	float waterHeight() const;
	float waterHeight(float valIfNoWater) const;
	float waterHeightWithStreams(float valIfNoWater) const;
	float waterDepth() const;
	void setWaterHeight(float waterHeight);
	void setWaterDepth(float waterDepth);
	bool hasWater() const;
	/***************************************************************************//**
	 * Sets the node's height to a given value. If above the current height, will
	 * use filler data to increase the height.
	 @param height The height to set to
	 @param fillerValue The data to use to fill the terrain if it is not heigh enough
	 @param maxHeight map maximum height
	 ******************************************************************************/
	void setHeight(float height, NodeMarker fillerValue, float& maxHeight);
	float getParticles() const;
	void setParticles(float particles);
	void setFoliageDensity(float density);
	float getFertility() const;
	float getFoliageDensity() const;
	float getFoliageWaterSupply() const;
protected:
	std::vector<NodeMarker> m_nodeData;
	WaterData m_waterData;
	VegetationData m_vegetationData;
};