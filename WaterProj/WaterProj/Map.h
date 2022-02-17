#pragma once

#include "Node.h"

class MapRenderer;

class Map 
{
public:
	Map(int width, int height);
	~Map();

	void render();
protected:
	Node* m_nodes;
	MapRenderer* m_renderer;
	int m_width;
	int m_height;
};