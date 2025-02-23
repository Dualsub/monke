#include "Game/Helpers/PerlinNoiseHelper.h"

namespace mk::PerlinNoiseHelper
{
    glm::vec2 RandomGradient(int32_t ix, int32_t iy, uint32_t seed)
    {
        const uint32_t w = 8 * sizeof(uint32_t);
        const uint32_t s = w / 2;
        uint32_t a = ix, b = iy;
        a *= 3284157443 + seed;

        b ^= a << s | a >> w - s;
        b *= 1911520717 + seed;

        a ^= b << s | b >> w - s;
        a *= 2048419325 + seed;
        float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

        glm::vec2 v;
        v.x = glm::sin(random);
        v.y = glm::cos(random);

        return v;
    }

    float DotGridGradient(int32_t ix, int32_t iy, float x, float y, uint32_t seed)
    {
        glm::vec2 gradient = RandomGradient(ix, iy, seed);

        float dx = x - (float)ix;
        float dy = y - (float)iy;

        return (dx * gradient.x + dy * gradient.y);
    }

    float Interpolate(float a0, float a1, float w)
    {
        return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
    }

    float Perlin(float x, float y, uint32_t seed)
    {
        int32_t x0 = (int32_t)x;
        int32_t y0 = (int32_t)y;
        int32_t x1 = x0 + 1;
        int32_t y1 = y0 + 1;

        float sx = x - (float)x0;
        float sy = y - (float)y0;

        float n0 = DotGridGradient(x0, y0, x, y, seed);
        float n1 = DotGridGradient(x1, y0, x, y, seed);
        float ix0 = Interpolate(n0, n1, sx);

        n0 = DotGridGradient(x0, y1, x, y, seed);
        n1 = DotGridGradient(x1, y1, x, y, seed);
        float ix1 = Interpolate(n0, n1, sx);

        float value = Interpolate(ix0, ix1, sy);

        return value;
    }
}