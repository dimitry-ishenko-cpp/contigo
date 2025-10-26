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
class image_base
{
protected:
    ////////////////////
    struct dim dim_;
    std::size_t stride_;
    D data_;

public:
    ////////////////////
    using color_type = D::element_type;
    static constexpr auto color_size = sizeof(color_type);

    constexpr image_base(struct dim dim, std::size_t stride, D data) :
        dim_{dim}, stride_{stride}, data_{std::move(data)}
    { }

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

template<typename C>
class image : public image_base<std::unique_ptr<C[]>>
{
public:
    ////////////////////
    explicit image(struct dim dim) : image_base<std::unique_ptr<C[]>>{dim,
        dim.width * this->color_size, std::make_unique_for_overwrite<C[]>(dim.width * dim.height)}
    { }

    image(struct dim dim, C c) : image{dim} { std::ranges::fill(this->span(), c); }
};

////////////////////////////////////////////////////////////////////////////////
template<typename D>
void fill(image_base<D>& img, pos pos, dim dim, typename image_base<D>::color_type c)
{
    clip_within(img.dim(), &pos, &dim);

    auto inc = img.stride() / img.color_size;
    auto pix = img.data() + pos.y * inc + pos.x;

    for (; dim.height; --dim.height, pix += inc) std::ranges::fill_n(pix, dim.width, c);
}

template<typename D, typename S>
void fill(image_base<D>& img, pos pos, const image_base<S>& src)
{
    auto img_pos = pos;
    auto img_dim = src.dim();
    clip_within(img.dim(), &img_pos, &img_dim);

    auto src_pos = -pos;
    auto src_dim = img.dim();
    clip_within(src.dim(), &src_pos, &src_dim);

    auto img_std = img.stride() / img.color_size;
    auto src_std = src.stride() / src.color_size;

    auto img_pix = img.data() + img_pos.y * img_std + img_pos.x;
    auto src_pix = src.data() + src_pos.y * src_std + src_pos.x;

    for (auto h = img_dim.height; h; --h, img_pix += img_std, src_pix += src_std)
        std::ranges::copy_n(src_pix, src_dim.width, img_pix);
}

////////////////////////////////////////////////////////////////////////////////
template<typename D, typename M>
void alpha_blend(image_base<D>& img, pos pos, const image_base<M>& src, typename image_base<D>::color_type c)
{
    static_assert(std::is_same_v<typename image_base<M>::color_type, shade>, "Unsupported mask image");

    auto img_pos = pos;
    auto img_dim = src.dim();
    clip_within(img.dim(), &img_pos, &img_dim);

    auto src_pos = -pos;
    auto src_dim = img.dim();
    clip_within(src.dim(), &src_pos, &src_dim);

    auto img_std = img.stride() / img.color_size;
    auto src_std = src.stride() / src.color_size;

    auto img_pix = img.data() + img_pos.y * img_std + img_pos.x;
    auto src_pix = src.data() + src_pos.y * src_std + src_pos.x;

    auto img_inc = img_std - img_dim.width;
    auto src_inc = src_std - src_dim.width;

    for (auto h = img_dim.height; h; --h, img_pix += img_inc, src_pix += src_inc)
        for (auto w = img_dim.width; w; --w, ++img_pix, ++src_pix)
            alpha_blend(*img_pix, c, *src_pix);
}
