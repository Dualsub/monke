#pragma once

#include "Core/Core.h"
#include <limits>

namespace ACS
{
    template <float t_duration, bool t_loop>
    class Timer
    {
    private:
        float m_time = t_duration;

    public:
        Timer() = default;
        Timer(bool start) : m_time(start ? t_duration : 0.0f) {}
        Timer(float time) : m_time(time) {}
        ~Timer() = default;

        bool Tick(float dt)
        {
            if (HasElapsed())
                return true;

            m_time -= dt;
            if (HasElapsed())
            {
                if (t_loop)
                {
                    m_time = t_duration;
                }
                else
                {
                    m_time = 0.0f;
                }

                return true;
            }

            return false;
        }

        void Reset(float time = t_duration)
        {
            m_time = time;
        }

        inline bool HasElapsed() const { return m_time <= 0.0f; }
        inline bool IsRunning() const { return m_time > 0.0f; }

        float GetTimeElapsed() const { return t_duration - m_time; }
        float GetTimeRemaining() const { return m_time; }
        float GetDuration() const { return t_duration; }
        float GetProgress() const { return 1.0f - m_time / t_duration; }
    };

    template <float t_duration>
    using LoopingTimer = Timer<t_duration, true>;

    template <float t_duration>
    using CooldownTimer = Timer<t_duration, false>;

    using DynamicTimer = Timer<std::numeric_limits<float>::max(), false>;
}