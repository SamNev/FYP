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
void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, std::vector<bool>* track)
{
    int ind = floor(pos.y) * dim.x + floor(pos.x);

    //nodes[ind].top()->color = glm::vec3(1.0f, 1.0f, 1.0f);
    //if (nodes[ind].waterDepth() > 0) return;

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

        track->at(offsetIndex) = true;
        //if (nodes[offsetIndex].waterDepth() > 0.1) 
         //   continue;

        float diff = glm::max(nodes[ind].topHeight() - nodes[offsetIndex].topHeight(), 0.001f);

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
    int prevIndex = index;
    //nodes[index].top()->color = glm::vec3(1.0f, 1.0f, 1.0f);

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

    m_speed *= 0.9f;

    if (particleEffect != glm::vec2(0.0f))
    {
        //particleEffect = glm::normalize(particleEffect);
        //m_speed += particleEffect * 0.5f;
    }
    m_speed += dir * 2.0f;

    if (glm::length(m_speed) < 0.001f)
        return false;

    m_pos += glm::normalize(m_speed) * (float)sqrt(2);
    index = (int)m_pos.y * dim.x + (int)m_pos.x;

    if (index == m_prevIndex)
        return false;

    m_prevIndex = prevIndex;

    if (m_pos.x < 0 || m_pos.x >= dim.x || m_pos.y < 0 || m_pos.y >= dim.y)
        return false;

    m_age++;
    cascade(m_pos, dim, nodes, track);
    return true;
}
#pragma optimize("", on);

bool Drop::flood(Node* nodes, glm::ivec2 dim) 
{
    while (m_volume > 0)
    {
        int index = (int)m_pos.y * dim.x + (int)m_pos.x;
        if (index < 0 || index >= dim.x * dim.y)
            return false;
        double plane = nodes[index].waterHeight(nodes[index].topHeight()) + 0.005f;

        std::stack<int> toTry;
        std::vector<int> set;
        int drain;
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

        std::function<bool(int, float&)> isValidDrain = [&](int i, float& drainHeight)
        {
            if (drainHeight > nodes[i].waterHeight(nodes[i].topHeight()))
            {
                if (std::find(set.begin(), set.end(), i + dim.x) != set.end() &&
                    std::find(set.begin(), set.end(), i - dim.x) != set.end() &&
                    std::find(set.begin(), set.end(), i + 1) != set.end() &&
                    std::find(set.begin(), set.end(), i - 1) != set.end() &&
                    std::find(set.begin(), set.end(), i + dim.x + 1) != set.end() &&
                    std::find(set.begin(), set.end(), i - dim.x - 1) != set.end() &&
                    std::find(set.begin(), set.end(), i + dim.x - 1) != set.end() &&
                    std::find(set.begin(), set.end(), i - dim.x + 1) != set.end())
                {
                    return false;
                }

                drain = i;
                drainHeight = nodes[i].waterHeight(nodes[i].topHeight());
                return true;
            }
            return false;
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
            int current = toTry.top();
            toTry.pop();
            fill(current, currVolume);
        }

        if (!set.empty() && currVolume < m_volume)
        {
            std::cout << "flooding set of " << set.size() << " nodes at " << m_pos.x << ", " << m_pos.y << " to height " << plane << std::endl;
            m_volume -= currVolume;

            for (int s : set)
            {
                nodes[s].setWaterHeight(plane);
            }
        }
        else if(!set.empty())
        {
            if (nodes[index].waterDepth() == 0.0f)
                break;

            set.clear();
            toTry.push(index); 
            currVolume = 0.0f;
            plane = nodes[index].waterHeight(nodes[index].topHeight());
            while (!toTry.empty())
            {
                int current = toTry.top();
                toTry.pop();
                fill(current, currVolume);
            }

            float drainHeight = FLT_MAX;
            for (int s : set)
            {
                isValidDrain(s, drainHeight);
            }

            glm::vec2 drainPos = glm::vec2(drain % dim.x, drain / dim.x);
            if (m_pos == drainPos)
            {
                nodes[drain].erodeByValue(0.005f);
                break;
            }

            for (int s : set)
            {
                nodes[s].setWaterHeight(nodes[drain].waterHeight(nodes[drain].topHeight()));
            }
            m_pos = drainPos;
            std::cout << "overflowing particle from set of " << set.size() << "nodes at " << m_pos.x << ", " << m_pos.y << ". plane = " << plane << " drain = " << drainHeight << std::endl;
        }

        delete[] tried;
    }
    return false;
}