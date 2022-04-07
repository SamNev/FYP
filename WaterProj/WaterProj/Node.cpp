#include "Node.h"

#include <ext.hpp>
#include <iostream>

NodeMarker* Node::top()
{
	return &m_nodeData[0];
}

void Node::addMarker(NodeMarker marker)
{
	float val = 0;
	//TODO: this needs to use the map max val
	addMarker(marker.height, marker.density, marker.hardStop, marker.color, val);
}

void Node::addMarker(float height, float density, bool hardStop, glm::vec3 color, float& maxHeight)
{
	if (height > maxHeight)
	{
		maxHeight = height;
	}

	// If there's no node data or this is the new lowest point, just push it back immediately.
	if (m_nodeData.size() == 0 || m_nodeData[m_nodeData.size() - 1].height >= height)
	{
		NodeMarker marker;
		marker.height = height;
		marker.density = density;
		marker.hardStop = hardStop;
		marker.color = color;

		m_nodeData.push_back(marker);
		return;
	}

	// Else, put it in the right position
	for (int i = 0; i < m_nodeData.size(); ++i)
	{
		if (height < m_nodeData[i].height)
			continue;

		NodeMarker marker;
		marker.height = height;
		marker.density = density;
		marker.hardStop = hardStop;
		marker.color = color;
		m_nodeData.insert(m_nodeData.begin() + i, marker);
		return;
	}
}

float Node::getDensityAtHeight(float height) const
{
	if (m_nodeData.size() == 0)
		return 0;

	float prevDensity = 0.0f;
	float prevHeight = 0.0f;
	float rockDensity = 0.0f;
	bool isRock = false;

	for (int i = 0; i < m_nodeData.size(); ++i)
	{
		if (height < m_nodeData[i].height)
		{
			if (!m_nodeData[i].hardStop) 
			{
				prevDensity = m_nodeData[i].density;
				prevHeight = m_nodeData[i].height;
			}
			else
			{
				if(!isRock)
					rockDensity = m_nodeData[i].density;
				isRock = !isRock;
			}
			continue;
		}

		if (isRock)
			return rockDensity;

		float currHeight = m_nodeData[i].height;
		float currDens = m_nodeData[i].density;
		float downScaledDist = (height - currHeight) / (prevHeight - currHeight);
		return (currDens + downScaledDist * (prevDensity - currDens));
	}

	return m_nodeData[m_nodeData.size() - 1].density;
}

glm::vec3 Node::getColorAtHeight(float height) const
{
	if (m_nodeData.size() == 0)
		return glm::vec3(0.0f, 0.0f, 0.0f);

	glm::vec3 prevColor(0.0f);
	float prevHeight = 0.0f;
	glm::vec3 rockColor(0.0f);
	bool isRock = false;

	for (int i = 0; i < m_nodeData.size(); ++i)
	{
		if (height < m_nodeData[i].height)
		{
			if (!m_nodeData[i].hardStop)
			{
				prevColor = m_nodeData[i].color;
				prevHeight = m_nodeData[i].height;
			}
			else
			{
				if (!isRock)
					rockColor = m_nodeData[i].color;
				isRock = !isRock;
			}
			continue;
		}

		if (isRock)
			return rockColor;

		float currHeight = m_nodeData[i].height;
		glm::vec3 currCol = m_nodeData[i].color;
		float downScaledDist = (height - currHeight) / (prevHeight - currHeight);
		return (currCol + downScaledDist * (prevColor - currCol));
	}

	return m_nodeData[0].color;
}

// debug function. Removes top node data.
void Node::skim()
{
	if(m_nodeData.size() > 1)
		m_nodeData.erase(m_nodeData.begin());
}

void Node::erodeByValue(float amount)
{
	const float base = m_nodeData[0].height;
	const float newVal = base - amount;

	for (int i = m_nodeData.size() - 1; i >= 0; --i)
	{
		if (m_nodeData[i].height < newVal)
			continue;

		if (i == 0)
		{
			m_nodeData[i].color = getColorAtHeight(newVal);
			m_nodeData[i].density = getDensityAtHeight(newVal);
			m_nodeData[i].height = newVal;
			return;
		}

		if (m_nodeData[i].height >= newVal)
		{
			m_nodeData[i].color = getColorAtHeight(newVal);
			m_nodeData[i].density = getDensityAtHeight(newVal);
			m_nodeData[i].height = newVal;
			m_nodeData.erase(m_nodeData.begin() + (i - 1));
			return;
		}
	}
}

float Node::topHeight() const
{
	return m_nodeData[0].height;
}

glm::vec3 Node::topColor() const
{
	return m_nodeData[0].color;
}

void Node::addWater(float height)
{
	if (height <= 0.0f)
		m_waterData.height = 0.0f;
	else
		m_waterData.height = height;
}

void Node::setWaterHeight(float waterHeight)
{
	float terrHeight = topHeight();
	if (terrHeight >= waterHeight)
		m_waterData.height = 0.0f;
	else
		m_waterData.height = waterHeight - terrHeight;
}

float Node::waterHeight(float valIfNoWater) const
{
	if (!hasWater())
		return valIfNoWater;

	return topHeight() + m_waterData.height + m_waterData.particles;
}

void Node::setWaterDepth(float waterDepth)
{
	m_waterData.height = waterDepth;
}

float Node::waterDepth() const
{
	return m_waterData.height;
}

bool Node::hasWater() const
{
	return m_waterData.height > 0.02f;
}

void Node::setHeight(float height, NodeMarker fillerData)
{
	fillerData.height = height;

	if (height < topHeight())
	{
		erodeByValue(topHeight() - height);
	}
	else if (height > topHeight())
	{
		addMarker(fillerData);
	}
}

float Node::getParticles()
{
	return m_waterData.particles;
}

void Node::setParticles(float particles)
{
	m_waterData.particles = particles;
}

float Node::getFoliageDensity()
{
	return m_waterData.particles;
}

void Node::setFoliageDensity(float foliageDensity)
{
	m_vegetationData.density = foliageDensity;
}