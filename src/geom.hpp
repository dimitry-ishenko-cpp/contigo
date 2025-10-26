////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <ostream>

////////////////////////////////////////////////////////////////////////////////
struct pos { int x, y; };
constexpr auto operator-(pos p) noexcept { return pos{-p.x, -p.y}; }

struct dim { unsigned width, height; };
constexpr auto operator/(dim x, dim y) noexcept { return dim{x.width / y.width, x.height / y.height}; }
inline auto& operator<<(std::ostream& os, dim dim) { return os << dim.width << 'x' << dim.height; }

constexpr void clip_within(dim rect, pos* pos, dim* dim) noexcept
{
    if (pos->x < 0)
    {
        dim->width = (dim->width > -pos->x) ? dim->width + pos->x : 0;
        pos->x = 0;
    }

    if (pos->y < 0)
    {
        dim->height = (dim->height > -pos->y) ? dim->height + pos->y : 0;
        pos->y = 0;
    }

    if (pos->x + dim->width > rect.width) dim->width = (pos->x < rect.width) ? rect.width - pos->x : 0;
    if (pos->y + dim->height > rect.height) dim->height = (pos->y < rect.height) ? rect.height - pos->y : 0;
}
