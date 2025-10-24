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
template<typename D>
class image
{
protected:
    ////////////////////
    struct dim dim_;
    std::size_t stride_;
    D data_;

    constexpr image(struct dim dim, std::size_t stride, D data) :
        dim_{dim}, stride_{stride}, data_{std::move(data)}
    { }

public:
    ////////////////////
    using color_type = D::element_type;
    static constexpr auto color_size = sizeof(color_type);

    constexpr auto dim() const noexcept { return dim_; }
    constexpr auto width() const noexcept { return dim().width; }
    constexpr auto height() const noexcept { return dim().height; }

    constexpr auto stride() const noexcept { return stride_; }

    constexpr auto size_bytes() const noexcept { return height() * stride(); }
    constexpr auto size() const noexcept { return size_bytes() / color_size; }

    constexpr auto data() noexcept { return data_.get(); }
    constexpr auto data() const noexcept { return data_.get(); }

    constexpr auto span() noexcept { return std::span{data(), size()}; }
    constexpr auto span() const noexcept { return std::span{data(), size()}; }
};

template<typename D>
inline auto bits_per_pixel<image<D>> = bits_per_pixel<typename image<D>::color_type>;

template<typename D>
inline auto depth<image<D>> = depth<typename image<D>::color_type>;

template<typename D>
inline auto num_colors<image<D>> = num_colors<typename image<D>::color_type>;

////////////////////////////////////////////////////////////////////////////////
template<typename C>
class pixmap : public image<std::unique_ptr<C[]>>
{
public:
    ////////////////////
    explicit pixmap(struct dim dim) : image<std::unique_ptr<C[]>>{dim,
        dim.width * this->color_size, std::make_unique_for_overwrite<C[]>(dim.width * dim.height)}
    { }

    pixmap(struct dim dim, C c) : pixmap{dim} { std::ranges::fill(this->span(), c); }
};

////////////////////////////////////////////////////////////////////////////////
template<typename D>
void fill(image<D>& img, pos pos, dim dim, typename image<D>::color_type c)
{
    clip_within(img.dim(), &pos, &dim);

    auto stride = img.stride() / img.color_size;
    auto px = img.data() + pos.y * stride + pos.x;

    for (; dim.height; --dim.height, px += stride) std::ranges::fill(px, px + dim.width, c);
}

////////////////////////////////////////////////////////////////////////////////
template<typename D, typename M,
    typename = std::enable_if_t<std::is_same_v<typename image<M>::color_type, shade>>
>
void alpha_blend(image<D>& img, pos pos, const image<M>& mask, typename image<D>::color_type c)
{
    auto mask_pos = pos;
    auto mask_dim = mask.dim();
    clip_within(img.dim(), &mask_pos, &mask_dim);

    auto img_pos = -pos;
    auto img_dim = img.dim();
    clip_within(mask.dim(), &img_pos, &img_dim);

    auto px = img.data() + mask_pos.y * img.width() + mask_pos.x;
    auto mx = mask.data() + img_pos.y * mask.width() + img_pos.x;

    auto pd = img.stride() / img.color_size - mask_dim.width;
    auto md = mask.stride() / mask.color_size - img_dim.width;

    for (auto h = mask_dim.height; h; --h, px += pd, mx += md)
        for (auto w = mask_dim.width; w; --w, ++px, ++mx) alpha_blend(*px, c, *mx);
}
