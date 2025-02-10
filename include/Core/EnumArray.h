#pragma once

#include <array>

namespace ACS
{
    template <typename Enum, typename T>
    class EnumArray
    {
    private:
        constexpr static size_t c_count = static_cast<size_t>(Enum::Count);
        using ArrayType = std::array<T, c_count>;

        ArrayType m_data;

    public:
        EnumArray() = default;
        ~EnumArray() = default;

        constexpr EnumArray(const T &value)
        {
            m_data.fill(value);
        }

        constexpr EnumArray(std::initializer_list<T> list)
        {
            // static_assert(list.size() == c_count, "Initializer list size must match enum count");

            size_t i = 0;
            for (const auto &value : list)
            {
                m_data[i++] = value;
            }
        }

        T &operator[](Enum e)
        {
            return m_data[static_cast<size_t>(e)];
        }

        const T &operator[](Enum e) const
        {
            return m_data[static_cast<size_t>(e)];
        }

        auto begin()
        {
            return m_data.begin();
        }

        auto end()
        {
            return m_data.end();
        }

        auto begin() const
        {
            return m_data.begin();
        }

        auto end() const
        {
            return m_data.end();
        }

        auto cbegin() const
        {
            return m_data.cbegin();
        }

        auto cend() const
        {
            return m_data.cend();
        }

        auto size() const
        {
            return m_data.size();
        }

        auto empty() const
        {
            return m_data.empty();
        }

        auto data()
        {
            return m_data.data();
        }
    };
}