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
template<typename T>
class bitmap
{
    struct dim dim_;
    unsigned stride_;

    std::unique_ptr<T[]> data_;

public:
    ////////////////////
    bitmap(struct dim dim, unsigned stride) :
        dim_{dim}, stride_{stride}, data_{std::make_unique<T[]>(dim.h * stride)}
    { }
    bitmap(struct dim dim) : bitmap{dim, dim.w} { }

    constexpr auto dim() const noexcept { return dim_; }
    constexpr auto stride() const noexcept { return stride_; }

    constexpr auto data() noexcept { return data_.get(); }

    constexpr auto size() const noexcept { return dim_.h * stride_; }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(T); }
};
