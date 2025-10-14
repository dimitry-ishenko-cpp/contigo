////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)

struct xrgb32
{
    union
    {
        std::uint32_t c;
        struct { std::uint8_t b, g, r; };
    };

    constexpr xrgb32() = default;
    constexpr xrgb32(std::uint8_t r, std::uint8_t g, std::uint8_t b) : b{b}, g{g}, r{r} { }
};

#pragma pack(pop)

using color = xrgb32;
