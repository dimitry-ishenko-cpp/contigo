////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <pixman.h>

////////////////////////////////////////////////////////////////////////////////
namespace pixman
{

using color = pixman_color;

struct image_delete { void operator()(pixman_image*); };
using image_ptr = std::unique_ptr<pixman_image, image_delete>;

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

class gray : public image_base
{
public:
    static constexpr unsigned depth = 8;
    static constexpr unsigned bits_per_pixel = 8;
    static constexpr unsigned num_colors = 1 << depth;

    gray(unsigned w, unsigned h);
};

class solid : public image_base
{
public:
    explicit solid(const color&);
};

class image : public image_base
{
public:
    static constexpr unsigned depth = 24;
    static constexpr unsigned bits_per_pixel = 32;
    static constexpr unsigned num_colors = 1 << depth;

    ////////////////////
    image(unsigned w, unsigned h);
    image(unsigned w, unsigned h, std::size_t stride, void* p);

    void fill(int x, int y, unsigned w, unsigned h, const color&);
    void fill(int x, int y, const image&);

    void alpha_blend(int x, int y, const gray&, const color&);
};

////////////////////////////////////////////////////////////////////////////////
}
