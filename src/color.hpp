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
template<typename C>
inline auto bits_per_pixel = sizeof(C) * std::numeric_limits<C>::digits;

template<>
inline auto bits_per_pixel<color> = (sizeof(color) - sizeof(color::x)) * std::numeric_limits<color>::digits;

template<typename C>
inline auto num_colors = 1 << bits_per_pixel<C>;

////////////////////////////////////////////////////////////////////////////////
template<typename C>
constexpr void alpha_blend(C& bg, C fg, shade mask)
{
    bg = (fg * mask + bg * (255 - mask)) / 255;
}

template<>
constexpr void alpha_blend<color>(color& bg, color fg, shade mask)
{
     alpha_blend(bg.b, fg.b, mask);
     alpha_blend(bg.g, fg.g, mask);
     alpha_blend(bg.r, fg.r, mask);
}
