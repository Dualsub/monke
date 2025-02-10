#include "Core/Core.h"

namespace mk
{
    uint32_t GetHandle(const std::string_view &input)
    {
        uint32_t hash = 0x811c9dc5;
        uint32_t prime = 0x1000193;

        for (char c : input)
        {
            hash ^= c;
            hash *= prime;
        }

        return hash;
    }
}