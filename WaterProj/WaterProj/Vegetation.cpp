#include "Vegetation.h"

#include "Node.h"

void Plant::grow() {
    size += rate * (maxsize - size);
};

void Plant::root(Node* nodes, glm::ivec2 dim, float f) {

    //Can always do this one
    nodes[index].setFoliageDensity(nodes[index].getFoliageDensity() + f);

    if (pos.x > 0) {
        //

        nodes[index - WSIZE].setFoliageDensity(nodes[index - WSIZE].getFoliageDensity() + f * 0.6); //(-1, 0)

        if (pos.y > 0)
            nodes[index - WSIZE - 1].setFoliageDensity(nodes[index - WSIZE - 1].getFoliageDensity() + f * 0.4);    //(-1, -1)

        if (pos.y < WSIZE - 1)
            nodes[index - WSIZE + 1].setFoliageDensity(nodes[index - WSIZE + 1].getFoliageDensity() + f * 0.4);    //(-1, 1)
    }

    if (pos.x < WSIZE - 1) {
        //
        nodes[index + WSIZE].setFoliageDensity(nodes[index + WSIZE].getFoliageDensity() + f * 0.6);    //(1, 0)

        if (pos.y > 0)
            nodes[index + WSIZE - 1].setFoliageDensity(nodes[index + WSIZE - 1].getFoliageDensity() + f * 0.4);    //(1, -1)

        if (pos.y < WSIZE - 1)
            nodes[index + WSIZE + 1].setFoliageDensity(nodes[index + WSIZE + 1].getFoliageDensity() + f * 0.4);    //(1, 1)
    }

    if (pos.y > 0)
        nodes[index - 1].setFoliageDensity(nodes[index - 1].getFoliageDensity() + f * 0.6);    //(0, -1)

    if (pos.y < WSIZE - 1)
        nodes[index + 1].setFoliageDensity(nodes[index + 1].getFoliageDensity() + f * 0.6);    //(0, 1)
}