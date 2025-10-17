////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"

////////////////////////////////////////////////////////////////////////////////
enum under_t { under_none, under_single, under_double, under_error };

struct cell
{
    static constexpr std::size_t max_chars = 6;

    char32_t chars[max_chars];
    unsigned width;

    bool bold;
    bool italic;
    bool strike;
    under_t under;

    color fg, bg;
};
