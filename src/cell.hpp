////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"

////////////////////////////////////////////////////////////////////////////////
enum underline { under_none, under_single, under_double, under_error };

struct cell
{
    static constexpr unsigned max_chars = 32;

    char chars[max_chars];
    unsigned width;

    bool bold;
    bool italic;
    bool strike;
    underline under;

    color fg, bg;
};
