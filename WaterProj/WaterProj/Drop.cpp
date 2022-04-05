#include "Drop.h"

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
void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes)
{
    int ind = pos.x * dim.y + pos.y;

    if (nodes[ind].waterDepth() > 0) return;

    // neighbors
    const int nx[8] = { -1,-1,-1, 0, 0, 1, 1, 1 };
    const int ny[8] = { -1, 0, 1,-1, 1,-1, 0, 1 };

    const float maxdiff = 0.01f;
    const float settling = 0.1f;

    for (int i = 0; i < 8; i++) 
    {
        glm::ivec2 offsetPos = (glm::ivec2)pos + glm::ivec2(nx[i], ny[i]);
        int offsetIndex = offsetPos.x * dim.y + offsetPos.y;
        float diff = (nodes[ind].topHeight() - nodes[offsetIndex].topHeight());

        if (offsetPos.x >= dim.x || offsetPos.y >= dim.y || offsetPos.x < 0 || offsetPos.y < 0)
            continue;
        if (nodes[offsetIndex].waterDepth() > 0) 
            continue;
        if (diff == 0)   
            continue;

        float excess = abs(diff) - maxdiff;
        if (excess <= 0)
            continue;

        float transfer = settling * excess / 2.0f;

        NodeMarker marker;
        if (diff > 0) {
            //TODO: this needs to take into account different kinds of sediment!
            nodes[ind].setHeight(nodes[ind].topHeight() - transfer, marker);
            nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() + transfer, marker);
        }
        else {
            nodes[ind].setHeight(nodes[ind].topHeight() + transfer, marker);
            nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() - transfer, marker);
        }
    }
}

bool Drop::descend(glm::vec3 norm, Node* nodes, std::vector<float>* track, glm::ivec2 dim, float scale) 
{
    if (m_volume < m_minVol)
        return false;
    
    int index = m_pos.x * dim.y + m_pos.y;
    // add volume to current node
    track->at(index) += m_volume;

    // deposition rate modified by plant density. Higher plant density->less erosion
    float modifiedDeposition = m_depositionRate * glm::max(1.0f - nodes[index].getFoliageDensity(), 0.0f);
    if (modifiedDeposition < 0) modifiedDeposition = 0;

    // TODO: investigate representation of friction
    float modifiedFriction = m_friction * (1.0 - nodes[index].getParticles());
    float modifiedEvaporationRate = m_evapRate * (1.0 - 0.2 * nodes[index].getParticles());

    if (glm::length(glm::vec2(norm.x, norm.z)) * modifiedFriction < 1E-5)
        return false;

    m_speed = glm::mix(glm::vec2(norm.x, norm.z), m_speed, modifiedFriction);
    m_speed = sqrt(2.0f) * normalize(m_speed);
    m_pos += m_speed;

    int newPos = (int)m_pos.x * dim.y + (int)m_pos.y;

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
{
    if (m_volume < m_minVol || m_remainingSpills-- <= 0)
        return false;

    std::vector<bool> tried;
    tried.reserve(dim.x * dim.y);
    std::fill(tried.begin(), tried.end(), false);

    std::unordered_map<int, float> boundary;
    std::vector<glm::ivec2> floodset;

    bool drainFound = false;
    glm::ivec2 drain;

    const std::function<bool(glm::ivec2, float)> findset = [&](glm::ivec2 pos, float plane) 
    {
        // bounds check
        if (pos.x < 0 || pos.y < 0 || pos.x >= dim.x || pos.y >= dim.y)
            return false;

        int index = pos.x * dim.y + pos.y;

        if (tried[index]) return true;
        tried[index] = true;

        // check wall/boundary
        if ((nodes[index].topHeight() + nodes[index].waterDepth()) > plane) 
        {
            boundary[index] = nodes[index].topHeight() + nodes[index].waterDepth();
            return true;
        }

        // find drain point
        if ((nodes[index].topHeight() + nodes[index].waterDepth()) < plane) 
        {

            if (!drainFound)
                drain = pos;

            else if (nodes[index].topHeight() + nodes[index].waterDepth() < nodes[drain.x * dim.y + drain.y].topHeight() + nodes[drain.x * dim.y + drain.y].waterDepth())
                drain = pos;

            drainFound = true;
            return false;
        }

        floodset.push_back(pos);

        if (!findset(pos + glm::ivec2(1, 0), plane)) return false;
        if (!findset(pos - glm::ivec2(1, 0), plane)) return false;
        if (!findset(pos + glm::ivec2(0, 1), plane)) return false;
        if (!findset(pos - glm::ivec2(0, 1), plane)) return false;
        if (!findset(pos + glm::ivec2(1, 1), plane)) return false;
        if (!findset(pos - glm::ivec2(1, 1), plane)) return false;
        if (!findset(pos + glm::ivec2(-1, 1), plane)) return false;
        if (!findset(pos - glm::ivec2(-1, 1), plane)) return false;

        return true;
    };

    glm::ivec2 pos = m_pos;
    int index = pos.x * dim.y + pos.y;
    float plane = nodes[index].topHeight() + nodes[index].waterDepth();

    std::pair<int, float> minBound = std::pair<int, float>(index, plane);

    while (m_volume > m_minVol && findset(pos, plane)) 
    {
        minBound = (*boundary.begin());
        for (auto& boundaryLocation : boundary)
        {
            if (boundaryLocation.second < minBound.second)
                minBound = boundaryLocation;
        }

        float height = m_volume * m_volumeFactor / (float)floodset.size();

        if (plane + height < minBound.second)
        {
            plane += height;
        }
        else 
        {
            m_volume -= (minBound.second - plane) / m_volumeFactor * (float)floodset.size();
            plane = minBound.second;
        }

        for (auto& s : floodset)
            nodes[s.x * dim.y + s.y].setWaterDepth(plane - nodes[s.x * dim.y + s.y].topHeight());

        boundary.erase(minBound.first);
        tried[minBound.first] = false;
        pos = glm::ivec2(minBound.first / dim.y, minBound.first % dim.y);

    }

    if (drainFound) 
    {
        // if we have a drain, cap water height to the level at that drain
        const std::function<void(glm::ivec2)> lowBound = [&](glm::ivec2 i) 
        {
            if (i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
                return;

            if (nodes[i.x * dim.y + i.y].waterDepth() == 0)
                return;

            if (nodes[i.x * dim.y + drain.y].topHeight() + nodes[i.x * dim.y + drain.y].waterDepth() < nodes[drain.x * dim.y + drain.y].topHeight() + nodes[drain.x * dim.y + drain.y].waterDepth())
                return;

            if (nodes[i.x * dim.y + i.y].topHeight() + nodes[i.x * dim.y + i.y].waterDepth() >= plane)
                return;

            plane = nodes[i.x * dim.y + i.y].topHeight() + nodes[i.x * dim.y + i.y].waterDepth();

        };

        lowBound(drain + glm::ivec2(1, 0));
        lowBound(drain - glm::ivec2(1, 0));
        lowBound(drain + glm::ivec2(0, 1));
        lowBound(drain - glm::ivec2(0, 1));
        lowBound(drain + glm::ivec2(1, 1));
        lowBound(drain - glm::ivec2(1, 1));
        lowBound(drain + glm::ivec2(-1, 1));
        lowBound(drain - glm::ivec2(-1, 1));

        for (auto& s : floodset) {
            int j = s.x * dim.y + s.y;
            nodes[j].setWaterDepth((plane > nodes[j].topHeight()) ? (plane - nodes[j].topHeight()) : 0.0);
        }

        for (auto& b : boundary) {
            int j = b.first;
            nodes[j].setWaterDepth((plane > nodes[j].topHeight()) ? (plane - nodes[j].topHeight()) : 0.0);
        }

        // TODO: distribute sediment
        m_sediment /= (float)floodset.size();
        m_pos = drain;

        return true;
    }

    return false;

}