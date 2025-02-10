#pragma once

#include "Core/Timer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <optional>

namespace mk
{
    constexpr uint32_t c_maxMessages = 16;
    constexpr float c_messageDuration = 5.0f;

    struct LogMessage
    {
        std::string message;
        float time = 0.0f;
        std::optional<glm::vec4> color = std::nullopt;
        std::optional<glm::vec3> position = std::nullopt;
    };

    class Logger
    {

    private:
        std::vector<LogMessage> m_messages;

        void AddMessage(const LogMessage &message)
        {
            if (m_messages.size() >= c_maxMessages)
            {
                m_messages.erase(m_messages.begin());
            }

            m_messages.push_back(message);
        }

        template <typename T>
        static std::string GetPrintable(const T &value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }

        template <>
        static std::string GetPrintable(const glm::vec2 &value)
        {
            std::stringstream ss;
            ss << "[" << std::setfill(' ') << std::setw(5) << value.x << ", " << std::setfill(' ') << std::setw(5) << value.y << "]";
            return ss.str();
        }

        template <>
        static std::string GetPrintable(const glm::vec3 &value)
        {
            std::stringstream ss;
            ss << "[" << std::setfill(' ') << std::setw(5) << value.x << ", " << std::setfill(' ') << std::setw(5) << value.y << ", " << std::setfill(' ') << std::setw(5) << value.z << "]";
            return ss.str();
        }

        template <>
        static std::string GetPrintable(const glm::vec4 &value)
        {
            std::stringstream ss;
            ss << "[" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << "]";
            return ss.str();
        }

    public:
        Logger() = default;
        ~Logger() = default;

        static Logger &GetInstance()
        {
            static Logger instance;
            return instance;
        }

        template <typename... Args>
        static void Print(Args &&...args)
        {
            std::stringstream ss;
            ((ss << GetPrintable(args) << " "), ...);
            LogMessage message{.message = ss.str(), .time = c_messageDuration};

            Logger &logger = GetInstance();
            logger.AddMessage(message);
        }

        template <typename... Args>
        static void PrintAt(const glm::vec3 &position, Args &&...args)
        {
            std::stringstream ss;
            ((ss << GetPrintable(args) << " "), ...);
            LogMessage message{.message = ss.str(), .time = c_messageDuration, .position = position};

            Logger &logger = GetInstance();
            logger.AddMessage(message);
        }

        static void Flush()
        {
            Logger &logger = GetInstance();
            logger.Clear();
        }

        void Update(float dt)
        {
            for (auto &message : m_messages)
            {
                message.time -= dt;
            }

            m_messages.erase(std::remove_if(m_messages.begin(), m_messages.end(), [](const LogMessage &message)
                                            { return message.time <= 0.0f; }),
                             m_messages.end());
        }

        void Clear() { m_messages.clear(); }
        const std::vector<LogMessage> &GetMessages() const { return m_messages; }
    };
}
