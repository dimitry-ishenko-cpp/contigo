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

////////////////////////////////////////////////////////////////////////////////
class bitmap
{
    struct dim dim_;
    unsigned stride_;

    std::unique_ptr<color[]> data_;

public:
    ////////////////////
    bitmap(struct dim dim, unsigned stride) :
        dim_{dim}, stride_{stride}, data_{std::make_unique<color[]>(dim_.height * stride_)}
    { }
    explicit bitmap(struct dim dim) : bitmap{dim, dim.width} { }

    bitmap(struct dim dim, unsigned stride, const color&);
    bitmap(struct dim dim, const color& color) : bitmap{dim, dim.width, color} { }

    ////////////////////
    constexpr auto width() const noexcept { return dim_.width; }
    constexpr auto height() const noexcept { return dim_.height; }

    constexpr auto dim() const noexcept { return dim_; }
    constexpr auto stride() const noexcept { return stride_; }

    auto data() noexcept { return data_.get(); }

    constexpr auto size() const noexcept { return dim_.height * stride_; }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(color); }
};

inline bitmap::bitmap(struct dim dim, unsigned stride, const color& color) :
    bitmap{dim, stride}
{
    std::fill_n(data(), size(), color);
}
