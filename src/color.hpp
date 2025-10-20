////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <limits>

////////////////////////////////////////////////////////////////////////////////
using shade = std::uint8_t;
constexpr auto shade_bits_per_pixel = sizeof(shade) * std::numeric_limits<shade>::digits;
constexpr auto shade_num_colors = 1 << shade_bits_per_pixel;

#pragma pack(push, 1)
struct color
{
    shade b, g, r, x;

    constexpr static auto bits_per_pixel = (sizeof(r) + sizeof(g) + sizeof(b)) * std::numeric_limits<color>::digits;
    constexpr static auto num_colors = 1 << bits_per_pixel;

    ////////////////////
    constexpr color() = default;
    constexpr color(shade r, shade g, shade b) : b{b}, g{g}, r{r}, x{} { }

    constexpr void alpha_blend(const color& c, shade mask)
    {
        b = (c.b * mask + b * (255 - mask)) / 255;
        g = (c.g * mask + g * (255 - mask)) / 255;
        r = (c.r * mask + r * (255 - mask)) / 255;
    }
};
#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////
constexpr color black{  0,   0,   0};
constexpr color red  {255,   0,   0};
constexpr color green{  0, 255,   0};
constexpr color blue {  0,   0, 255};
constexpr color white{255, 255, 255};
