#include "Drop.h"
#include <iostream>
#include <stack>
#include "Node.h"

Drop::Drop(glm::vec2 pos) 
{ 
    m_pos = pos; 
}

Drop::Drop(glm::vec2 pos, glm::ivec2 dim, float volume) 
{
    m_pos = pos;
    m_volume = volume;
}

//TODO: change the awful nx/ny thing
#pragma optimize("", off);
void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes)
{
    int ind = floor(pos.y) * dim.x + floor(pos.x);

    if (nodes[ind].waterDepth() > 0) return;

    // neighbors
    const int nx[8] = { -1,-1,-1, 0, 0, 1, 1, 1 };
    const int ny[8] = { -1, 0, 1,-1, 1,-1, 0, 1 };

    const float maxDiff = 0.01f;
    //0.1f
    const float settling = 0.1f;

    for (int i = 0; i < 8; i++) 
    {
        glm::ivec2 offsetPos = (glm::ivec2)pos + glm::ivec2(nx[i], ny[i]);
        int offsetIndex = offsetPos.y * dim.x + offsetPos.x;

        if (offsetPos.x >= dim.x || offsetPos.y >= dim.y || offsetPos.x < 0 || offsetPos.y < 0)
            continue;
        if (nodes[offsetIndex].waterDepth() > 0) 
            continue;

        float diff = (nodes[ind].topHeight() - nodes[offsetIndex].topHeight());

        if (diff == 0)   
            continue;

        float excess = abs(diff) - maxDiff;
        if (excess <= 0)
            continue;

        float transfer = settling * excess / 2.0f;

        NodeMarker marker;
        if (diff > 0) {
            //TODO: this needs to take into account different kinds of sediment!
            nodes[ind].setHeight(nodes[ind].topHeight() - transfer, marker);
            //nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() + transfer, marker);
        }
        else {
            //nodes[ind].setHeight(nodes[ind].topHeight() + transfer, marker);
            nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() - transfer, marker);
        }
    }
}
#pragma optimize("", on);

bool Drop::descend(glm::vec3 norm, Node* nodes, std::vector<float>* track, glm::ivec2 dim, float scale) 
{
    if (m_volume < m_minVol)
        return false;
    
    int index = floor(m_pos.y) * dim.x + floor(m_pos.x);
    // add volume to current node
    track->at(index) += m_volume;

    // deposition rate modified by plant density. Higher plant density->less erosion
    float modifiedDeposition = m_depositionRate * glm::max(1.0f - nodes[index].getFoliageDensity(), 0.01f);

    // TODO: investigate representation of friction
    float modifiedFriction = m_friction * 1.0 - nodes[index].getParticles();
    float modifiedEvaporationRate = m_evapRate * (1.0 - 0.2 * nodes[index].getParticles());

    // was 1e-5
    if (glm::length(glm::vec2(norm.x, norm.z)) * modifiedFriction < 1E-5)
        return false;

    m_speed = glm::mix(glm::vec2(norm.x, norm.z), m_speed, modifiedFriction);
    m_speed = sqrt(2.0f) * normalize(m_speed);
    m_pos += m_speed;

    int newPos = floor(m_pos.y) * dim.x + floor(m_pos.x);

    // OOB check
    if (!glm::all(glm::greaterThanEqual(m_pos, glm::vec2(0))) || !glm::all(glm::lessThan((glm::ivec2)m_pos, dim))) 
    {
        m_volume = 0.0;
        return false;
    }

    // ignore if in pool
    if (nodes[newPos].waterDepth() > 0.0) 
    {
        return false;
    }

    // sediment transfer
    float heightDiff = nodes[index].topHeight() - nodes[newPos].topHeight();
    if (heightDiff < 0) heightDiff = 0;
    float modifiedDiff = heightDiff - m_sediment;
    m_sediment += modifiedDeposition * modifiedDiff;
    //TODO: proper sediment values;
    NodeMarker node;
    nodes[index].setHeight(nodes[index].topHeight() - (modifiedDeposition * modifiedDiff), node);

    /*
    //Mass-Transfer (in MASS)
    float heightDiff = h[index] - h[nind];
    if (heightDiff < 0) heightDiff = 0;//max(0.0, (h[index]-h[nind]));
    float modifiedDiff = heightDiff - sediment;
    sediment += effD * modifiedDiff;
    h[index] -= effD * modifiedDiff;
    */

    m_sediment /= (1.0 - modifiedEvaporationRate);
    m_volume *= (1.0 - modifiedEvaporationRate);

    cascade(m_pos, dim, nodes);

    m_age++;
    return true;
}

bool Drop::flood(Node* nodes, glm::ivec2 dim) 
{ //Current Height
    int index = (int)m_pos.y * dim.x + (int)m_pos.x;
    double plane = nodes[index].waterHeight(nodes[index].topHeight());
    double initialplane = plane;

    std::stack<int> toTry;
    //Floodset
    std::vector<int> set;
    int fail = 10;

    //Iterate
    while (m_volume > m_minVol && fail) {

        set.clear();
        const int size = (int)dim.x * dim.y;
        bool* tried = new bool[size];

        for (int i = 0; i < size; ++i) {
            tried[i] = false;
        }
        int drain;
        bool drainfound = false;

        std::function<bool(int)> inBounds = [&](int i)
        {
            if (i < 0 || i >= size)
                return false;

            if (tried[i])
                return false;

            tried[i] = true;

            return true;
        };

        std::function<void(int)> fill = [&](int i) {

            if (plane < nodes[i].waterHeight(nodes[i].topHeight())) {
                return;
            }

            //Drainage Point
            if (initialplane > nodes[i].waterHeight(nodes[i].topHeight())) {

                //No Drain yet
                if (!drainfound)
                    drain = i;

                //Lower Drain
                else if (nodes[drain].waterHeight(nodes[drain].topHeight()) > nodes[i].waterHeight(nodes[i].topHeight()))
                    drain = i;

                drainfound = true;
                return;
            }

            //Part of the Pool
            set.push_back(i);

            if(inBounds(i + dim.x))
                toTry.push(i + dim.x);
            if (inBounds(i - dim.x))
                toTry.push(i - dim.x);
            if (inBounds(i + 1))
                toTry.push(i + 1);
            if (inBounds(i - 1))
                toTry.push(i - 1);
            if (inBounds(i + dim.x + 1))
                toTry.push(i + dim.x + 1);
            if (inBounds(i - dim.x - 1))
                toTry.push(i - dim.x - 1);
            if (inBounds(i + dim.x - 1))
                toTry.push(i + dim.x - 1);
            if (inBounds(i - dim.x + 1))
                toTry.push(i - dim.x + 1);
        };

        //Perform Flood
        toTry.push(index);
        while (!toTry.empty())
        {
            fill(toTry.top());
            toTry.pop();
        }

        delete[] tried;

        //Drainage Point
        if (drainfound) {

            //Set the Drop Position and Evaporate
            glm::vec2 pos = glm::vec2(drain / dim.y, drain % dim.y);

            //Set the New Waterlevel (Slowly)
            double drainage = 0.001;
            plane = (1.0 - drainage) * initialplane + drainage * (nodes[drain].waterHeight(nodes[drain].topHeight()));

            std::cout << "Drain found, setting height to " << plane << ". Volume is " << m_volume << std::endl;
            //Compute the New Height
            for (auto& s : set)
                nodes[s].setWaterDepth((plane > nodes[s].topHeight()) ? (plane - nodes[s].topHeight()) : 0.0);

            //Remove Sediment
            m_sediment *= 0.1;
            break;
        }

        //Get Volume under Plane
        double tVol = 0.0;
        for (auto& s : set)
            tVol += m_volumeFactor * (plane - (nodes[s].waterHeight(nodes[s].topHeight())));

        //We can partially fill this volume
        if (tVol <= m_volume && initialplane < plane) {

            //Raise water level to plane height
            for (auto& s : set)
                nodes[s].setWaterDepth(plane - nodes[s].topHeight());

            //Adjust Drop Volume
            m_volume -= tVol;
            tVol = 0.0;
        }

        //Plane was too high.
        else fail--;

        //Adjust Planes
        initialplane = (plane > initialplane) ? plane : initialplane;
        plane += 0.5 * (m_volume - tVol) / (double)set.size() / m_volumeFactor;
    }

    //Couldn't place the volume (for some reason)- so ignore this drop.
    if (fail == 0)
        m_volume = 0.0;

}