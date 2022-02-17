#include "Map.h"
#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"

Map::Map(int width, int height)
{
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_renderer = new MapRenderer();

	PerlinNoise perlin;
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			m_nodes[y * width + x].addMarker(perlin.noise(x, y, 0.5f), 1.0f);
		}
	}
}

void Map::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			if (x + 1 != m_width && y + 1 != m_height)
			{
				const glm::vec3 current = { x, y, m_nodes[y * m_width + x].top()->height };
				const glm::vec3 down = { x, y + 1, m_nodes[(y + 1) * m_width + x].top()->height };
				const glm::vec3 right = { x + 1, y, m_nodes[y * m_width + (x + 1)].top()->height };
				const glm::vec3 diag = { x + 1, y + 1, m_nodes[(y + 1) * m_width + (x + 1)].top()->height };

				glm::vec3 trisPositions[6] = { current, right, down, right, diag, down };

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(6, GL_FLOAT, 0, trisPositions);

				glPushMatrix();
				glTranslatef(-1.0f, 0.0f, -6.0f);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glPopMatrix();

				glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
	}
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
	delete(m_renderer);
}