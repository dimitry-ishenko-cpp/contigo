////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "geom.hpp"
#include <memory>

////////////////////////////////////////////////////////////////////////////////
template<typename C>
class bitmap
{
    struct dim dim_;
    unsigned stride_;

    std::unique_ptr<C[]> data_;

public:
    ////////////////////
    bitmap(struct dim dim, unsigned stride) :
        dim_{dim}, stride_{stride}, data_{std::make_unique<C[]>(dim_.height * stride_)}
    { }
    explicit bitmap(struct dim dim) : bitmap{dim, dim.width} { }

    constexpr auto width() const noexcept { return dim_.width; }
    constexpr auto height() const noexcept { return dim_.height; }

    constexpr auto dim() const noexcept { return dim_; }
    constexpr auto stride() const noexcept { return stride_; }

    constexpr auto data() noexcept { return data_.get(); }

    constexpr auto size() const noexcept { return dim_.height * stride_; }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(C); }
};
