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
template<typename C>
class image
{
    struct dim dim_;
    std::unique_ptr<C[]> data_;

public:
    ////////////////////
    constexpr auto dim() const noexcept { return dim_; }
    constexpr auto width() const noexcept { return dim().width; }
    constexpr auto height() const noexcept { return dim().height; }

    constexpr auto size() const noexcept { return width() * height(); }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(C); }

    constexpr auto data() noexcept { return data_.get(); }
    constexpr auto data() const noexcept { return data_.get(); }

    constexpr auto span() noexcept { return std::span{data(), size()}; }
    constexpr auto span() const noexcept { return std::span{data(), size()}; }

    ////////////////////
    image(struct dim dim) : dim_{dim}, data_{std::make_unique<C[]>(size())} { }
    image(struct dim dim, C c) : image{dim} { std::ranges::fill(span(), c); }
};

template<typename C>
inline auto bits_per_pixel<image<C>> = bits_per_pixel<C>;

////////////////////////////////////////////////////////////////////////////////
template<typename C>
void fill(image<C>& img, pos pos, dim dim, C c)
{
    clip_within(img.dim(), &pos, &dim);

    auto px = img.data() + pos.y * img.width() + pos.x;
    for (; dim.height; --dim.height, px += img.width()) std::ranges::fill(px, px + dim.width, c);
}

////////////////////////////////////////////////////////////////////////////////
template<typename C>
void alpha_blend(image<C>& img, pos pos, const image<shade>& mask, C c)
{
    auto mask_pos = pos;
    auto mask_dim = mask.dim();
    clip_within(img.dim(), &mask_pos, &mask_dim);

    auto img_pos = -pos;
    auto img_dim = img.dim();
    clip_within(mask.dim(), &img_pos, &img_dim);

    auto px = img.data() + mask_pos.y * img.width() + mask_pos.x;
    auto mx = mask.data() + img_pos.y * mask.width() + img_pos.x;

    auto pd = img.width() - mask_dim.width;
    auto md = mask.width() - img_dim.width;

    for (auto h = mask_dim.height; h; --h, px += pd, mx += md)
        for (auto w = mask_dim.width; w; --w, ++px, ++mx) alpha_blend(*px, c, *mx);
}
