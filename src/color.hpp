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

template<typename C>
inline unsigned bits_per_pixel = sizeof(C) * std::numeric_limits<C>::digits;

template<typename C>
inline unsigned depth = bits_per_pixel<C>;

template<typename C>
inline unsigned num_colors = 1 << depth<C>;

////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
struct color
{
    shade b, g, r, x;

    constexpr color() noexcept = default;
    constexpr color(shade r, shade g, shade b) noexcept : b{b}, g{g}, r{r}, x{} { }
};
#pragma pack(pop)

constexpr bool operator==(color x, color y) noexcept { return x.b == y.b && x.g == y.g && x.r == y.r; }

constexpr color black{  0,   0,   0};
constexpr color red  {255,   0,   0};
constexpr color green{  0, 255,   0};
constexpr color blue {  0,   0, 255};
constexpr color white{255, 255, 255};

////////////////////////////////////////////////////////////////////////////////
template<>
inline unsigned bits_per_pixel<color> = sizeof(color) * bits_per_pixel<decltype(color::x)>;

template<>
inline unsigned depth<color> = bits_per_pixel<color> - bits_per_pixel<decltype(color::x)>;

////////////////////////////////////////////////////////////////////////////////
template<typename C>
constexpr inline void alpha_blend(C& bg, C fg, shade mask)
{
    auto bg0 = fg * mask + bg * (255 - mask);
    bg = (bg0 + 1 + (bg0 >> 8)) >> 8;
}

template<>
constexpr inline void alpha_blend<color>(color& bg, color fg, shade mask)
{
    auto not_mask = 255 - mask;

    auto b0 = fg.b * mask + bg.b * not_mask;
    bg.b = (b0 + 1 + (b0 >> 8)) >> 8;

    auto g0 = fg.g * mask + bg.g * not_mask;
    bg.g = (g0 + 1 + (g0 >> 8)) >> 8;

    auto r0 = fg.r * mask + bg.r * not_mask;
    bg.r = (r0 + 1 + (r0 >> 8)) >> 8;
}
