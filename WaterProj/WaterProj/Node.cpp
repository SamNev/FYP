#include "Node.h"

#include <ext.hpp>
#include <iostream>

NodeMarker* Node::top()
{
	return &m_nodeData[0];
}

void Node::addMarker(NodeMarker marker, float& maxHeight)
{
	addMarker(marker.height, marker.resistiveForce, marker.hardStop, marker.color, marker.fertility, marker.sandAmount, marker.clayAmount, maxHeight);
}

void Node::addMarker(float height, float resistiveForce, bool hardStop, glm::vec3 color, float fertility, float sandAmount, float clayAmount, float& maxHeight)
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
		marker.resistiveForce = resistiveForce;
		marker.hardStop = hardStop;
		marker.fertility = fertility;
		marker.color = color;
		marker.sandAmount = sandAmount;
		marker.clayAmount = clayAmount;

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
		marker.resistiveForce = resistiveForce;
		marker.hardStop = hardStop;
		marker.fertility = fertility;
		marker.color = color;
		marker.sandAmount = sandAmount;
		marker.clayAmount = clayAmount;

		m_nodeData.insert(m_nodeData.begin() + i, marker);
		return;
	}
}

float Node::getResistiveForceAtHeight(float height) const
{
	if (m_nodeData.size() == 0)
		return 0;

	float prevResist = 0.0f;
	float prevHeight = 0.0f;
	float rockResist = 0.0f;
	bool isRock = false;

	for (int i = 0; i < m_nodeData.size(); ++i)
	{
		if (height < m_nodeData[i].height)
		{
			if (!m_nodeData[i].hardStop) 
			{
				prevResist = m_nodeData[i].resistiveForce;
				prevHeight = m_nodeData[i].height;
			}
			else
			{
				if(!isRock)
					rockResist = m_nodeData[i].resistiveForce;
				isRock = !isRock;
			}
			continue;
		}

		if (isRock)
			return rockResist;

		float currHeight = m_nodeData[i].height;
		float currDens = m_nodeData[i].resistiveForce;
		float downScaledDist = (height - currHeight) / (prevHeight - currHeight);
		return (currDens + downScaledDist * (prevResist - currDens));
	}

	return m_nodeData[m_nodeData.size() - 1].resistiveForce;
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
			m_nodeData[i].resistiveForce = getResistiveForceAtHeight(newVal);
			m_nodeData[i].height = newVal;
			return;
		}

		if (m_nodeData[i].height >= newVal)
		{
			m_nodeData[i].color = getColorAtHeight(newVal);
			m_nodeData[i].resistiveForce = getResistiveForceAtHeight(newVal);
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
	float particles = glm::min(1.0f, glm::max(0.0f, getParticles() - 1.0f));

	if(particles > 0.01f && m_waterData.height < 0.2f)
		return m_nodeData[0].color * glm::max(0.25f, 1.0f - particles) + glm::vec3(0.0f, 0.2f, 0.9f) * glm::min(0.75f, particles);

	if (getFoliageDensity() == 0.0f)
		return m_nodeData[0].color;
	else
		return m_nodeData[0].color * (1.0f - getFoliageDensity()) + glm::vec3(0.1f, 0.7f, 0.0f) * getFoliageDensity();
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

float Node::waterHeight() const
{
	if (!hasWater())
		return topHeight();

	return topHeight() + m_waterData.height;
}

float Node::waterHeight(float valIfNoWater) const
{
	if (!hasWater())
		return valIfNoWater;

	return topHeight() + m_waterData.height;
}

float Node::waterHeightWithStreams(float valIfNoWater) const
{
	if (!hasWater() && m_waterData.particles == 0)
		return valIfNoWater;

	return topHeight() + m_waterData.height + glm::min(1.0f, m_waterData.particles);
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
	return m_waterData.height > 0.00f;
}

void Node::setHeight(float height, NodeMarker fillerData, float& maxHeight)
{
	NodeMarker copy = fillerData;
	copy.height = height;

	if (height < topHeight())
	{
		erodeByValue(topHeight() - height);
	}
	else if (height > topHeight())
	{
		addMarker(copy, maxHeight);
	}
}

NodeMarker Node::getDataAboveHeight(float height, bool ignoreRock) const
{
	NodeMarker marker = m_nodeData[0];
	float currentAmount = 0.0f;

	for (int i = 1; i < m_nodeData.size(); i++)
	{
		if (height < m_nodeData[i - 1].height && height < m_nodeData[i].height)
		{
			if (!ignoreRock || !m_nodeData[i].hardStop)
			{
				float amount = abs(m_nodeData[i - 1].height - m_nodeData[i].height);
				currentAmount += amount;
				if (currentAmount == 0.0f)
					continue;
				marker.mix(m_nodeData[i], amount / currentAmount);
			}
		}
		else if (height < m_nodeData[i - 1].height)
		{
			if (!ignoreRock || !m_nodeData[i].hardStop)
			{
				float amount = m_nodeData[i - 1].height - height;
				currentAmount += amount;
				if (currentAmount == 0.0f)
					continue;
				marker.mix(m_nodeData[i], amount / currentAmount);
			}
		}
		else
			break;
	}

	return marker;
}

float Node::getParticles() const
{
	return m_waterData.particles;
}

void Node::setParticles(float particles)
{
	m_waterData.particles = particles;
}

float Node::getFoliageDensity() const
{
	return glm::min(1.0f, m_vegetationData.density);
}

void Node::setFoliageDensity(float foliageDensity)
{
	float modifDensity = glm::min(1.0f, glm::max(foliageDensity, 0.0f));
	m_vegetationData.density = modifDensity;
}