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

struct color
{
    using value_type = std::uint8_t;

    union
    {
        struct { value_type b, g, r; };
        std::uint32_t c;
    };

    constexpr color() = default;
    constexpr color(value_type r, value_type g, value_type b) : b{b}, g{g}, r{r} { }
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////
constexpr color black{  0,   0,   0};
constexpr color red  {255,   0,   0};
constexpr color green{  0, 255,   0};
constexpr color blue {  0,   0, 255};
constexpr color white{255, 255, 255};
