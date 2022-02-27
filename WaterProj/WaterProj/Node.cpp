#include "Node.h"

NodeMarker* Node::top()
{
	return &m_nodeData[0];
}

void Node::addMarker(float height, float density, float& maxHeight)
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
		m_nodeData.insert(m_nodeData.begin() + i, marker);
		return;
	}
}