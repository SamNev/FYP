#pragma once

#include "Node.h"

class MapRenderer;

class Map 
{
public:
	Map(int width, int height);
	~Map();

	void render();
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	Node* getNodeAt(int x, int y);
	float getHeightAt(int x, int y);
protected:
	Node* m_nodes;
	MapRenderer* m_renderer;
	int m_width;
	int m_height;
};