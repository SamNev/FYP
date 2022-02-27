#include "Node.h"

#include <ext.hpp>

NodeMarker* Node::top()
{
	return &m_nodeData[0];
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

float Node::getDensityAtHeight(float height)
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

// debug function. Removes top node data.
void Node::skim()
{
	if(m_nodeData.size() > 1)
		m_nodeData.erase(m_nodeData.begin());
}