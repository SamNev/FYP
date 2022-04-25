#include "Plant.h"

#include "Node.h"

Plant::Plant(int i, glm::ivec2 d) 
{
    m_index = i;
    m_pos = glm::vec2(i / d.y, i % d.y);
};

Plant::Plant(glm::vec2 p, glm::ivec2 d) 
{
    m_pos = p;
    m_index = (int)p.x * d.y + (int)p.y;
};

void Plant::grow() 
{
    m_size += m_rate * (m_maxSize - m_size);
};

void Plant::root(Node* nodes, glm::ivec2 dim, float f) 
{
    float fertility = glm::max(0.0f, nodes[m_index].top()->fertility);
    nodes[m_index].setFoliageDensity(nodes[m_index].getFoliageDensity() + f * fertility);

    if (m_pos.x > 0) {
        nodes[m_index - dim.x].setFoliageDensity(nodes[m_index - dim.x].getFoliageDensity() + f * 0.6 * fertility); //(-1, 0)

        if (m_pos.y > 0)
            nodes[m_index - dim.x - 1].setFoliageDensity(nodes[m_index - dim.x - 1].getFoliageDensity() + f * 0.4 * fertility);    //(-1, -1)

        if (m_pos.y < dim.x - 1)
            nodes[m_index - dim.x + 1].setFoliageDensity(nodes[m_index - dim.x + 1].getFoliageDensity() + f * 0.4 * fertility);    //(-1, 1)
    }

    if (m_pos.x < dim.x - 1) {
        nodes[m_index + dim.x].setFoliageDensity(nodes[m_index + dim.x].getFoliageDensity() + f * 0.6 * fertility);    //(1, 0)

        if (m_pos.y > 0)
            nodes[m_index + dim.x - 1].setFoliageDensity(nodes[m_index + dim.x - 1].getFoliageDensity() + f * 0.4 * fertility);    //(1, -1)

        if (m_pos.y < dim.x - 1)
            nodes[m_index + dim.x + 1].setFoliageDensity(nodes[m_index + dim.x + 1].getFoliageDensity() + f * 0.4 * fertility);    //(1, 1)
    }

    if (m_pos.y > 0)
        nodes[m_index - 1].setFoliageDensity(nodes[m_index - 1].getFoliageDensity() + f * 0.6 * fertility);    //(0, -1)

    if (m_pos.y < dim.x - 1)
        nodes[m_index + 1].setFoliageDensity(nodes[m_index + 1].getFoliageDensity() + f * 0.6 * fertility);    //(0, 1)
}