#include "Plant.h"

#include "Node.h"

void Plant::root(Node* nodes, glm::ivec2 dim, glm::ivec2 pos, float f) 
{
    int index = pos.y * dim.x + pos.x;
    float fertility = glm::max(0.0f, nodes[index].top()->fertility);
    nodes[index].setFoliageDensity(nodes[index].getFoliageDensity() + f * fertility);

    if (pos.y > 0) {
        nodes[index - dim.x].setFoliageDensity(nodes[index - dim.x].getFoliageDensity() + f * 0.6 * fertility);

        if (pos.x > 0)
            nodes[index - dim.x - 1].setFoliageDensity(nodes[index - dim.x - 1].getFoliageDensity() + f * 0.4 * fertility);

        if (pos.x < dim.x - 1)
            nodes[index - dim.x + 1].setFoliageDensity(nodes[index - dim.x + 1].getFoliageDensity() + f * 0.4 * fertility); 
    }

    if (pos.y < dim.y - 1) {
        nodes[index + dim.x].setFoliageDensity(nodes[index + dim.x].getFoliageDensity() + f * 0.6 * fertility);

        if (pos.x > 0)
            nodes[index + dim.x - 1].setFoliageDensity(nodes[index + dim.x - 1].getFoliageDensity() + f * 0.4 * fertility);

        if (pos.x < dim.x - 1)
            nodes[index + dim.x + 1].setFoliageDensity(nodes[index + dim.x + 1].getFoliageDensity() + f * 0.4 * fertility); 
    }

    if (pos.x > 0)
        nodes[index - 1].setFoliageDensity(nodes[index - 1].getFoliageDensity() + f * 0.6 * fertility); 

    if (pos.x < dim.x - 1)
        nodes[index + 1].setFoliageDensity(nodes[index + 1].getFoliageDensity() + f * 0.6 * fertility);
}