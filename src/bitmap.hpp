////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"
#include "geom.hpp"

#include <algorithm>
#include <memory>
#include <span>

////////////////////////////////////////////////////////////////////////////////
class bitmap
{
    struct dim dim_;
    std::unique_ptr<color[]> data_;

public:
    ////////////////////
    constexpr auto width() const noexcept { return dim_.width; }
    constexpr auto height() const noexcept { return dim_.height; }
    constexpr auto dim() const noexcept { return dim_; }

    constexpr auto size() const noexcept { return dim_.width * dim_.height; }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(color); }

    auto pix() noexcept { return std::span{data_.get(), size()}; }

    ////////////////////
    explicit bitmap(struct dim dim) :
        dim_{dim}, data_{std::make_unique<color[]>(size())}
    { }
    bitmap(struct dim dim, const color& c) : bitmap{dim} { std::ranges::fill(pix(), c); }
};
