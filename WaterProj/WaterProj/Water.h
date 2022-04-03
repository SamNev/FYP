#include <functional>
#include <GL/glew.h>
#include <glm.hpp>
#include <unordered_map>

struct Drop {
    //Construct Particle at Position
    Drop(glm::vec2 _pos) { m_pos = _pos; }
    Drop(glm::vec2 _p, glm::ivec2 dim, float v) {
        m_pos = _p;
        m_volume = v;
    }

    //Properties
    int m_age = 0;
    glm::vec2 m_pos;
    glm::vec2 m_speed = glm::vec2(0.0);
    float m_volume = 1.0;   //This will vary in time
    float m_sediment = 0.0; //Sediment concentration

    //Parameters
    const float m_density = 1.0;  //This gives varying amounts of inertia and stuff...
    const float m_evapRate = 0.001;
    const float m_depositionRate = 1.2 * 0.08;
    const float m_minVol = 0.01;
    const float m_friction = 0.25;
    const float m_volumeFactor = 0.5; //"Water Deposition Rate"

    //Number of Spills Left
    int m_remainingSpills = 0;

    bool descend(glm::vec3 norm, Node* n, float* path, float* track, float* pd, glm::ivec2 dim, float scale);
    bool flood(Node* n, glm::ivec2 dim);

    static void cascade(glm::vec2 pos, glm::ivec2 dim, Node* n) {

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

};

// b is pool here, no clue why
//while(drop.descend(normal((int)drop.pos.x * dim.y + (int)drop.pos.y), heightmap, waterpath, waterpool, track, plantdensity, dim, SCALE));
bool Drop::descend(glm::vec3 norm, Node* n, float* p, float* track, float* pd, glm::ivec2 dim, float scale) {

    if (m_volume < m_minVol)
        return false;

    //Initial Position
    glm::ivec2 ipos = m_pos;
    int ind = ipos.x * dim.y + ipos.y;

    //Add to Path
    track[ind] += m_volume;

    //Effective Parameter Set
    /* Higher plant density means less erosion */
    float effD = m_depositionRate * 1.0 - pd[ind];//max(0.0, );
    if (effD < 0) effD = 0;

    /* Higher Friction, Lower Evaporation in Streams
    makes particles prefer established streams -> "curvy" */

    float effF = m_friction * (1.0 - p[ind]);
    float effR = m_evapRate * (1.0 - 0.2 * p[ind]);

    //Particle is Not Accelerated
    if (glm::length(glm::vec2(norm.x, norm.z)) * effF < 1E-5)
        return false;

    m_speed = glm::mix(glm::vec2(norm.x, norm.z), m_speed, effF);
    m_speed = sqrt(2.0f) * normalize(m_speed);
    m_pos += m_speed;

    //New Position
    int nind = (int)m_pos.x * dim.y + (int)m_pos.y;

    //Out-Of-Bounds
    if (!glm::all(glm::greaterThanEqual(m_pos, glm::vec2(0))) ||
        !glm::all(glm::lessThan((glm::ivec2)m_pos, dim))) {
        m_volume = 0.0;
        return false;
    }

    //Particle is in Pool
    if (n[nind].waterDepth() > 0.0) {
        return false;
    }

    //Mass-Transfer (in MASS)
    float c_eq = n[ind].topHeight() - n[nind].topHeight();
    if (c_eq < 0) c_eq = 0;//max(0.0, (h[ind]-h[nind]));
    float cdiff = c_eq - m_sediment;
    m_sediment += effD * cdiff;
    //TODO: proper sediment values;
    NodeMarker node;
    n[ind].setHeight(n[ind].topHeight() - (effD * cdiff), node);

    //Evaporate (Mass Conservative)
    m_sediment /= (1.0 - effR);
    m_volume *= (1.0 - effR);

    cascade(m_pos, dim, n);

    m_age++;
    return true;

}

/*

Flooding Algorithm Overhaul:
  Currently, I can only flood at my position as long as we are rising.
  Then I return and let the particle descend. This should only happen if I can't find a closed set to fill.

  So: Rise and fill, removing the m_volume as we go along.
  Then: If we find a lower point, try to rise and fill from there.

*/

bool Drop::flood(Node* n, glm::ivec2 dim) {

    if (m_volume < m_minVol || m_remainingSpills-- <= 0)
        return false;

    //Either try to find a closed set under this plane, which has a certain m_volume,
    //or raise the plane till we find the correct closed set height.
    //And only if it can't be found, re-emit the particle.

    std::vector<bool> tried;
    tried.reserve(dim.x * dim.y);
    std::fill(tried.begin(), tried.end(), false);

    std::unordered_map<int, float> boundary;
    std::vector<glm::ivec2> floodset;

    bool drainfound = false;
    glm::ivec2 drain;

    //Returns whether the set is closed at given height

    const std::function<bool(glm::ivec2, float)> findset = [&](glm::ivec2 i, float plane) {

        if (i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
            return true;

        int ind = i.x * dim.y + i.y;

        if (tried[ind]) return true;
        tried[ind] = true;

        //Wall / Boundary
        if ((n[ind].topHeight() + n[ind].waterDepth()) > plane) {
            boundary[ind] = n[ind].topHeight() + n[ind].waterDepth();
            return true;
        }

        //Drainage Point
        if ((n[ind].topHeight() + n[ind].waterDepth()) < plane) {

            //No Drain yet
            if (!drainfound)
                drain = i;
            
            //Lower Drain
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

        //Find the Lowest Element on the Boundary
        minbound = (*boundary.begin());
        for (auto& b : boundary)
            if (b.second < minbound.second)
                minbound = b;

        //Compute the Height of our Volume over the Set
        float vheight = m_volume * m_volumeFactor / (float)floodset.size();

        //Not High Enough: Fill 'er up
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

        //Search for Exposed Neighbor with Non-Zero Waterlevel
        const std::function<void(glm::ivec2)> lowbound = [&](glm::ivec2 i) {

            //Out-Of-Bounds
            if (i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
                return;

            if (n[i.x * dim.y + i.y].waterDepth() == 0)
                return;

            //Below Drain Height
            if (n[i.x * dim.y + drain.y].topHeight() + n[i.x * dim.y + drain.y].waterDepth() < n[drain.x * dim.y + drain.y].topHeight() + n[drain.x * dim.y + drain.y].waterDepth())
                return;

            //Higher than Plane (we want lower)
            if (n[i.x * dim.y + i.y].topHeight() + n[i.x * dim.y + i.y].waterDepth() >= plane)
                return;

            plane = n[i.x * dim.y + i.y].topHeight() + n[i.x * dim.y + i.y].waterDepth();

        };

        lowbound(drain + glm::ivec2(1, 0));    //Fill Neighbors
        lowbound(drain - glm::ivec2(1, 0));    //Fill Neighbors
        lowbound(drain + glm::ivec2(0, 1));    //Fill Neighbors
        lowbound(drain - glm::ivec2(0, 1));    //Fill Neighbors
        lowbound(drain + glm::ivec2(1, 1));    //Fill Neighbors
        lowbound(drain - glm::ivec2(1, 1));    //Fill Neighbors
        lowbound(drain + glm::ivec2(-1, 1));    //Fill Neighbors
        lowbound(drain - glm::ivec2(-1, 1));    //Fill Neighbors

        float oldvolume = m_volume;

        //Water-Level to Plane-Height
        for (auto& s : floodset) {
            int j = s.x * dim.y + s.y;
            //  m_volume += ((plane > h[ind])?(h[ind] + p[ind] - plane):p[ind])/m_volumeFactor;
            n[j].setWaterDepth((plane > n[j].topHeight()) ? (plane - n[j].topHeight()) : 0.0);
        }

        for (auto& b : boundary) {
            int j = b.first;
            //  m_volume += ((plane > h[ind])?(h[ind] + p[ind] - plane):p[ind])/m_volumeFactor;
            n[j].setWaterDepth((plane > n[j].topHeight()) ? (plane - n[j].topHeight()) : 0.0);
        }

        //    m_sediment *= oldvolume/m_volume;
        m_sediment /= (float)floodset.size(); //Distribute Sediment in Pool
        m_pos = drain;

        return true;

    }

    return false;

}