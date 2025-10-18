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

template<typename T>
inline auto bits_per_pixel = sizeof(T) * std::numeric_limits<T>::digits;

template<>
inline auto bits_per_pixel<xrgb> = (sizeof(xrgb) - sizeof(xrgb::x)) * std::numeric_limits<mono>::digits;

template<typename T>
inline auto num_colors = 1 << bits_per_pixel<T>;
