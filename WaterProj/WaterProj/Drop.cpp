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

bool Drop::descend(glm::vec3 norm, Node* nodes, std::vector<bool>* track, glm::ivec2 dim, float scale) 
{
    if (m_volume < m_minVol)
        return false;

    int index = (int)m_pos.y * dim.x + (int)m_pos.x;

    if (index < 0 || index >= dim.x * dim.y)
        return false;

    glm::vec2 dir = glm::vec2(norm.x, norm.y);

    nodes[index].setParticles(nodes[index].getParticles() + m_volume);

    glm::vec2 particleEffect(0.0f);
    if(index - dim.x > 0)
        particleEffect.y -= nodes[index - dim.x].getParticles();
    if (index + dim.x < dim.x * dim.y)
        particleEffect.y += nodes[index + dim.x].getParticles();
    if(index - 1 > 0)
        particleEffect.x -= nodes[index - 1].getParticles();
    if (index + 1 < dim.x * dim.y)
        particleEffect.x += nodes[index + 1].getParticles();

    m_speed *= 0.5f;

    if (particleEffect != glm::vec2(0.0f))
    {
        particleEffect = glm::normalize(particleEffect);
        m_speed += particleEffect * 2.0f;
    }
    m_speed += dir * 2.0f;

    if (glm::length(m_speed) < 0.01f)
        return false;

    m_pos += glm::normalize(m_speed) * (float)sqrt(2);

    if (m_pos.x < 0 || m_pos.x >= dim.x || m_pos.y < 0 || m_pos.y >= dim.y)
        return false;

    m_age++;
    cascade(m_pos, dim, nodes);
    return true;
}
#pragma optimize("", on);

bool Drop::flood(Node* nodes, glm::ivec2 dim) 
{
    int index = (int)m_pos.y * dim.x + (int)m_pos.x;
    double plane = nodes[index].waterHeight(nodes[index].topHeight()) + 0.005f;
    
    std::stack<int> toTry;
    std::vector<int> set;

    while (m_volume > 0)
    {
        set.clear();
        const int size = (int)dim.x * dim.y;
        bool* tried = new bool[size];

        std::fill(tried, tried + size, false);

        std::function<bool(int)> inBounds = [&](int i)
        {
            if (i < 0 || i >= size)
                return false;

            if (tried[i])
                return false;

            tried[i] = true;

            return true;
        };

        std::function<void(int, float&)> fill = [&](int i, float& vol) 
        {
            if (plane < nodes[i].waterHeight(nodes[i].topHeight())) {
                return;
            }

            //Part of the Pool
            set.push_back(i);
            vol += plane - nodes[i].waterHeight(nodes[i].topHeight());

            if (inBounds(i + dim.x))
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
        if (inBounds(index))
            toTry.push(index);
        else
            break;

        float currVolume = 0.0f;
        while (!toTry.empty())
        {
            fill(toTry.top(), currVolume);
            toTry.pop();
        }

        if (!set.empty() && set.size() * 0.005f < m_volume)
        {
            std::cout << "flooding" << std::endl;
            m_volume -= set.size() * 0.005f;

            for (int s : set)
            {
                nodes[s].setWaterDepth(nodes[s].waterDepth() + 0.005f);
            }
        }
        else 
        {
            std::cout << "should spawn particle with vol " << m_volume << std::endl;
            return false;
        }

        delete[] tried;
    }
    return false;
}