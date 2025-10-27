////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
using gray8 = std::uint8_t;

#pragma pack(push, 1)
struct xrgb32
{
    gray8 b, g, r, x;

    constexpr xrgb32() noexcept = default;
    constexpr xrgb32(gray8 r, gray8 g, gray8 b) noexcept : b{b}, g{g}, r{r}, x{} { }
};
#pragma pack(pop)

constexpr bool operator==(xrgb32 x, xrgb32 y) noexcept { return x.b == y.b && x.g == y.g && x.r == y.r; }
