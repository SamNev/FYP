#include "Drop.h"

#include "Node.h"

Drop::Drop(glm::vec2 pos) 
{ 
    m_pos = pos; 
}

Drop::Drop(glm::vec2 p, glm::ivec2 dim, float v) 
{
    m_pos = p;
    m_volume = v;
}

void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* n)
{
    glm::ivec2 ipos = pos;
    int ind = ipos.x * dim.y + ipos.y;

    if (n[ind].waterDepth() > 0) return; //Don't do this with water

    //Neighbor Positions (8-Way)
    const int nx[8] = { -1,-1,-1, 0, 0, 1, 1, 1 };
    const int ny[8] = { -1, 0, 1,-1, 1,-1, 0, 1 };

    const float maxdiff = 0.01f;
    const float settling = 0.1f;

    //Iterate over all Neighbors
    for (int m = 0; m < 8; m++) {

        glm::ivec2 npos = ipos + glm::ivec2(nx[m], ny[m]);
        int nind = npos.x * dim.y + npos.y;

        if (npos.x >= dim.x || npos.y >= dim.y
            || npos.x < 0 || npos.y < 0) continue;

        if (n[nind].waterDepth() > 0) continue; //Don't do this with water

        //Full Height-Different Between Positions!
        float diff = (n[ind].topHeight() - n[nind].topHeight());
        if (diff == 0)   //No Height Difference
            continue;

        //The Amount of Excess Difference!
        float excess = abs(diff) - maxdiff;
        if (excess <= 0)  //No Excess
            continue;

        //Actual Amount Transferred
        float transfer = settling * excess / 2.0f;

        NodeMarker marker;
        //Cap by Maximum Transferrable Amount
        if (diff > 0) {
            //TODO: this needs to take into account different kinds of sediment!
            n[ind].setHeight(n[ind].topHeight() - transfer, marker);
            n[nind].setHeight(n[nind].topHeight() + transfer, marker);
        }
        else {
            n[ind].setHeight(n[ind].topHeight() + transfer, marker);
            n[nind].setHeight(n[nind].topHeight() - transfer, marker);
        }

    }
}

bool Drop::descend(glm::vec3 norm, Node* nodes, std::vector<float>* track, glm::ivec2 dim, float scale) {

    if (m_volume < m_minVol)
        return false;

    // initial Position
    glm::ivec2 ipos = m_pos;
    int ind = ipos.x * dim.y + ipos.y;
    track->at(ind) += m_volume;

    float effD = m_depositionRate * 1.0 - nodes[ind].getFoliageDensity();//max(0.0, );
    if (effD < 0) effD = 0;

    float effF = m_friction * (1.0 - nodes[ind].getParticles());
    float effR = m_evapRate * (1.0 - 0.2 * nodes[ind].getParticles());

    if (glm::length(glm::vec2(norm.x, norm.z)) * effF < 1E-5)
        return false;

    m_speed = glm::mix(glm::vec2(norm.x, norm.z), m_speed, effF);
    m_speed = sqrt(2.0f) * normalize(m_speed);
    m_pos += m_speed;

    int newPos = (int)m_pos.x * dim.y + (int)m_pos.y;

    // OOB check
    if (!glm::all(glm::greaterThanEqual(m_pos, glm::vec2(0))) ||
        !glm::all(glm::lessThan((glm::ivec2)m_pos, dim))) {
        m_volume = 0.0;
        return false;
    }

    // ignore if in pool
    if (nodes[newPos].waterDepth() > 0.0) {
        return false;
    }

    // sediment transfer
    float c_eq = nodes[ind].topHeight() - nodes[newPos].topHeight();
    if (c_eq < 0) c_eq = 0;
    float cdiff = c_eq - m_sediment;
    m_sediment += effD * cdiff;
    //TODO: proper sediment values;
    NodeMarker node;
    nodes[ind].setHeight(nodes[ind].topHeight() - (effD * cdiff), node);

    m_sediment /= (1.0 - effR);
    m_volume *= (1.0 - effR);

    cascade(m_pos, dim, nodes);

    m_age++;
    return true;

}

bool Drop::flood(Node* n, glm::ivec2 dim) {

    if (m_volume < m_minVol || m_remainingSpills-- <= 0)
        return false;

    std::vector<bool> tried;
    tried.reserve(dim.x * dim.y);
    std::fill(tried.begin(), tried.end(), false);

    std::unordered_map<int, float> boundary;
    std::vector<glm::ivec2> floodset;

    bool drainfound = false;
    glm::ivec2 drain;

    const std::function<bool(glm::ivec2, float)> findset = [&](glm::ivec2 i, float plane) {

        if (i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
            return true;

        int ind = i.x * dim.y + i.y;

        if (tried[ind]) return true;
        tried[ind] = true;

        // check wall/boundary
        if ((n[ind].topHeight() + n[ind].waterDepth()) > plane) {
            boundary[ind] = n[ind].topHeight() + n[ind].waterDepth();
            return true;
        }

        // find drain point
        if ((n[ind].topHeight() + n[ind].waterDepth()) < plane) {

            if (!drainfound)
                drain = i;

            else if (n[ind].topHeight() + n[ind].waterDepth() < n[drain.x * dim.y + drain.y].topHeight() + n[drain.x * dim.y + drain.y].waterDepth())
                drain = i;

            drainfound = true;
            return false;

        }

        floodset.push_back(i);

        if (!findset(i + glm::ivec2(1, 0), plane)) return false;
        if (!findset(i - glm::ivec2(1, 0), plane)) return false;
        if (!findset(i + glm::ivec2(0, 1), plane)) return false;
        if (!findset(i - glm::ivec2(0, 1), plane)) return false;
        if (!findset(i + glm::ivec2(1, 1), plane)) return false;
        if (!findset(i - glm::ivec2(1, 1), plane)) return false;
        if (!findset(i + glm::ivec2(-1, 1), plane)) return false;
        if (!findset(i - glm::ivec2(-1, 1), plane)) return false;

        return true;
    };

    glm::ivec2 ipos = m_pos;
    int ind = ipos.x * dim.y + ipos.y;
    float plane = n[ind].topHeight() + n[ind].waterDepth();

    std::pair<int, float> minbound = std::pair<int, float>(ind, plane);

    while (m_volume > m_minVol && findset(ipos, plane)) {

        minbound = (*boundary.begin());
        for (auto& b : boundary)
            if (b.second < minbound.second)
                minbound = b;

        float vheight = m_volume * m_volumeFactor / (float)floodset.size();

        if (plane + vheight < minbound.second)
            plane += vheight;

        else {
            m_volume -= (minbound.second - plane) / m_volumeFactor * (float)floodset.size();
            plane = minbound.second;
        }

        for (auto& s : floodset)
            n[s.x * dim.y + s.y].setWaterDepth(plane - n[s.x * dim.y + s.y].topHeight());

        boundary.erase(minbound.first);
        tried[minbound.first] = false;
        ipos = glm::ivec2(minbound.first / dim.y, minbound.first % dim.y);

    }

    if (drainfound) {

        const std::function<void(glm::ivec2)> lowbound = [&](glm::ivec2 i) {

            if (i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
                return;

            if (n[i.x * dim.y + i.y].waterDepth() == 0)
                return;

            if (n[i.x * dim.y + drain.y].topHeight() + n[i.x * dim.y + drain.y].waterDepth() < n[drain.x * dim.y + drain.y].topHeight() + n[drain.x * dim.y + drain.y].waterDepth())
                return;

            if (n[i.x * dim.y + i.y].topHeight() + n[i.x * dim.y + i.y].waterDepth() >= plane)
                return;

            plane = n[i.x * dim.y + i.y].topHeight() + n[i.x * dim.y + i.y].waterDepth();

        };

        lowbound(drain + glm::ivec2(1, 0));
        lowbound(drain - glm::ivec2(1, 0));
        lowbound(drain + glm::ivec2(0, 1));
        lowbound(drain - glm::ivec2(0, 1));
        lowbound(drain + glm::ivec2(1, 1));
        lowbound(drain - glm::ivec2(1, 1));
        lowbound(drain + glm::ivec2(-1, 1));
        lowbound(drain - glm::ivec2(-1, 1));

        for (auto& s : floodset) {
            int j = s.x * dim.y + s.y;
            n[j].setWaterDepth((plane > n[j].topHeight()) ? (plane - n[j].topHeight()) : 0.0);
        }

        for (auto& b : boundary) {
            int j = b.first;
            n[j].setWaterDepth((plane > n[j].topHeight()) ? (plane - n[j].topHeight()) : 0.0);
        }

        // TODO: distribute sediment
        m_sediment /= (float)floodset.size();
        m_pos = drain;

        return true;

    }

    return false;

}