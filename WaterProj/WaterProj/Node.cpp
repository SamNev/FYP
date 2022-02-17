#include "Node.h"

NodeMarker* Node::top()
{
	return &m_nodeData[0];
}

void Node::addMarker(float height, float density)
{
	NodeMarker marker;
	marker.height = height;
	marker.density = density;

	m_nodeData.push_back(marker);
}