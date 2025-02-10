#pragma once

#include "glm/glm.hpp"

#include <vector>
#include <numeric>
#include <array>
#include <limits>
#include <algorithm>

namespace ACS
{
    class Bounds
    {
    private:
        glm::vec2 m_min = glm::vec2(std::numeric_limits<float>::infinity());
        glm::vec2 m_max = glm::vec2(-std::numeric_limits<float>::infinity());

    public:
        Bounds() = default;
        Bounds(const glm::vec2 &min, const glm::vec2 &max)
            : m_min(min), m_max(max) {}
        ~Bounds() = default;

        const glm::vec2 &GetMin() const { return m_min; }
        const glm::vec2 &GetMax() const { return m_max; }
        const glm::vec2 GetCenter() const { return (m_min + m_max) * 0.5f; }

        void Fit(const glm::vec2 &point)
        {
            m_min = (glm::min)(m_min, point);
            m_max = (glm::max)(m_max, point);
        }

        template <typename Iterator>
        void Fit(Iterator begin, Iterator end)
        {
            for (auto it = begin; it != end; ++it)
            {
                Fit(*it);
            }
        }

        bool Intersects(const Bounds &other) const
        {
            return !(m_max.x < other.m_min.x || m_min.x > other.m_max.x ||
                     m_max.y < other.m_min.y || m_min.y > other.m_max.y);
        }

        bool Contains(const glm::vec2 &point) const
        {
            return point.x >= m_min.x && point.x <= m_max.x &&
                   point.y >= m_min.y && point.y <= m_max.y;
        }
    };

    class Grid
    {
    public:
        using NodeId = uint32_t;
        static constexpr NodeId InvalidNodeId = NodeId(-1);

        struct Node
        {
            Bounds bounds = {};
            std::array<std::array<NodeId, 2>, 2> children = {InvalidNodeId, InvalidNodeId, InvalidNodeId, InvalidNodeId};

            bool IsLeaf() const
            {
                return children[0][0] == InvalidNodeId && children[0][1] == InvalidNodeId &&
                       children[1][0] == InvalidNodeId && children[1][1] == InvalidNodeId;
            }
        };

        struct Point
        {
            glm::vec2 position = {};
            uint32_t index = 0;
        };

    private:
        static constexpr uint32_t MaxDepth = 8;

        Bounds m_bounds = {};
        NodeId m_root = InvalidNodeId;
        std::vector<Point> m_points = {};
        std::vector<Node> m_nodes = {};
        std::vector<uint32_t> m_nodePointsBegin = {};

        template <typename Iterator>
        NodeId Build(const Bounds &bounds, Iterator begin, Iterator end, uint32_t depth = 0)
        {
            if (begin == end)
            {
                return InvalidNodeId;
            }

            m_nodes.emplace_back();
            NodeId nodeId = m_nodes.size() - 1;
            Node &node = m_nodes.back();
            node.bounds = bounds;

            m_nodePointsBegin.push_back(begin - m_points.begin());

            if (begin + 1 == end || depth == MaxDepth)
            {
                return nodeId;
            }

            glm::vec2 center = bounds.GetCenter();

            Iterator splitY = std::partition(begin, end, [&](const Point &point)
                                             { return point.position.y < center.y; });

            Iterator splitXLower = std::partition(begin, splitY, [&](const Point &point)
                                                  { return point.position.x < center.x; });

            Iterator splitXUpper = std::partition(splitY, end, [&](const Point &point)
                                                  { return point.position.x < center.x; });

            m_nodes[nodeId].children[0][0] = Build(Bounds(bounds.GetMin(), center), begin, splitXLower, depth + 1);
            m_nodes[nodeId].children[0][1] = Build(Bounds(glm::vec2(center.x, bounds.GetMin().y), glm::vec2(bounds.GetMax().x, center.y)), splitXLower, splitY, depth + 1);
            m_nodes[nodeId].children[1][0] = Build(Bounds(glm::vec2(bounds.GetMin().x, center.y), glm::vec2(center.x, bounds.GetMax().y)), splitY, splitXUpper, depth + 1);
            m_nodes[nodeId].children[1][1] = Build(Bounds(center, bounds.GetMax()), splitXUpper, end, depth + 1);

            return nodeId;
        }

    public:
        Grid() = default;
        ~Grid() = default;

        void Build(const std::vector<glm::vec3> &points)
        {
            Clear();
            m_points.resize(points.size());
            for (uint32_t i = 0; i < points.size(); i++)
            {
                m_points[i].position = glm::vec2(points[i].x, points[i].z);
                m_points[i].index = i;
                m_bounds.Fit(m_points[i].position);
            }
            m_root = Build(m_bounds, m_points.begin(), m_points.end());
            m_nodePointsBegin.push_back(m_points.size());
        }

        std::vector<uint32_t> QueryIndices(const Bounds &bounds) const
        {
            if (!m_bounds.Intersects(bounds))
            {
                return {};
            }

            if (m_root == InvalidNodeId)
            {
                return {};
            }

            std::vector<uint32_t> result;
            std::vector<NodeId> stack;

            stack.push_back(m_root);

            while (!stack.empty())
            {
                NodeId nodeId = stack.back();
                stack.pop_back();

                if (nodeId == InvalidNodeId)
                {
                    continue;
                }

                const Node &node = m_nodes[nodeId];
                // Check if the bounds intersect
                if (!node.bounds.Intersects(bounds))
                {
                    continue;
                }

                if (node.IsLeaf())
                {
                    for (uint32_t i = m_nodePointsBegin[nodeId]; i < m_nodePointsBegin[nodeId + 1]; i++)
                    {
                        if (!bounds.Contains(m_points[i].position))
                        {
                            continue;
                        }

                        result.push_back(m_points[i].index);
                    }
                }
                else
                {
                    stack.push_back(node.children[0][0]);
                    stack.push_back(node.children[0][1]);
                    stack.push_back(node.children[1][0]);
                    stack.push_back(node.children[1][1]);
                }
            }

            return result;
        }

        void Clear()
        {
            m_bounds = {};
            m_root = InvalidNodeId;
            m_points.clear();
            m_nodes.clear();
            m_nodePointsBegin.clear();
        }

        const std::vector<Node> &GetNodes() const { return m_nodes; }
        const std::vector<Point> &GetPoints() const { return m_points; }
        const Point &GetPoint(uint32_t index) const { return m_points[index]; }
        const std::vector<uint32_t> &GetNodePointsBegin() const { return m_nodePointsBegin; }
    };
}