////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"
#include "geom.hpp"

#include <memory>
#include <pixman.h>

////////////////////////////////////////////////////////////////////////////////
namespace pixman
{

using color = pixman_color;
using image_ptr = std::unique_ptr<pixman_image, int(*)(pixman_image*)>;

////////////////////////////////////////////////////////////////////////////////
class image_base
{
public:
    ////////////////////
    unsigned width() const;
    unsigned height() const;

    std::size_t stride() const;

    void* data();

protected:
    ////////////////////
    image_ptr img_;
    friend class image;

    image_base(image_ptr img) : img_{std::move(img)} { }
};

////////////////////////////////////////////////////////////////////////////////
class gray : public image_base
{
public:
    static constexpr unsigned depth = 8;
    static constexpr unsigned bits_per_pixel = 8;
    static constexpr unsigned num_colors = 1 << depth;

    explicit gray(struct dim);
};

////////////////////////////////////////////////////////////////////////////////
class solid : public image_base
{
public:
    explicit solid(xrgb32);
};

////////////////////////////////////////////////////////////////////////////////
class image : public image_base
{
public:
    static constexpr unsigned depth = 24;
    static constexpr unsigned bits_per_pixel = 32;
    static constexpr unsigned num_colors = 1 << depth;

    ////////////////////
    explicit image(struct dim);
    image(struct dim, std::size_t stride, void* data);

    void fill(pos, struct dim, xrgb32);
    void fill(pos, const image&);

    void alpha_blend(pos, const gray&, xrgb32);
};

////////////////////////////////////////////////////////////////////////////////
}
