#pragma once

// Undefine windows.h define for min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define GLM_ENABLE_EXPERIMENTAL

#include <string_view>
#include <cstdint>
#include <numeric>
#include <limits>

namespace mk
{
    uint32_t GetHandle(const std::string_view &input);

    template <typename T>
    constexpr std::string_view GetTypeName()
    {
        std::string_view name = typeid(T).name();
        size_t pos = name.find_last_of(':');
        return name.substr(pos + 1);
    }

    template <typename T>
    constexpr T Infinity()
    {
        return std::numeric_limits<T>::infinity();
    }
}