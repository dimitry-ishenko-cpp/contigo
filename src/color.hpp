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
using mono = std::uint8_t;

#pragma pack(push, 1)
struct xrgb
{
    mono b, g, r, x;

    constexpr xrgb() = default;
    constexpr xrgb(mono r, mono g, mono b) : b{b}, g{g}, r{r}, x{} { }
};
#pragma pack(pop)

template<typename C>
inline auto bits_per_pixel = sizeof(C) * std::numeric_limits<C>::digits;

template<>
inline auto bits_per_pixel<xrgb> = (sizeof(xrgb) - sizeof(xrgb::x)) * std::numeric_limits<mono>::digits;

template<typename C>
inline auto num_colors = 1 << bits_per_pixel<C>;

////////////////////////////////////////////////////////////////////////////////
constexpr xrgb black{  0,   0,   0};
constexpr xrgb red  {255,   0,   0};
constexpr xrgb green{  0, 255,   0};
constexpr xrgb blue {  0,   0, 255};
constexpr xrgb white{255, 255, 255};
