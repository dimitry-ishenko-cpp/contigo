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

struct image_delete { void operator()(pixman_image* image) { pixman_image_unref(image); } };
using image_ptr = std::unique_ptr<pixman_image, image_delete>;

////////////////////////////////////////////////////////////////////////////////
class image_base
{
public:
    ////////////////////
    unsigned width() const { return pixman_image_get_width(&*pix_); }
    unsigned height() const { return pixman_image_get_height(&*pix_); }

    std::size_t stride() const { return pixman_image_get_stride(&*pix_); }

    template<typename D>
    auto data() { return reinterpret_cast<D>(pixman_image_get_data(&*pix_)); }

protected:
    ////////////////////
    image_ptr pix_;
    friend struct image;

    image_base(image_ptr pix) : pix_{std::move(pix)} { }
};

////////////////////////////////////////////////////////////////////////////////
struct gray : image_base
{
    static constexpr unsigned depth = 8;
    static constexpr unsigned bits_per_pixel = 8;
    static constexpr unsigned num_colors = 1 << depth;

    gray(unsigned w, unsigned h) :
        image_base{image_ptr{pixman_image_create_bits(PIXMAN_a8, w, h, nullptr, 0)}}
    { }
};

////////////////////////////////////////////////////////////////////////////////
struct image : image_base
{
    static constexpr unsigned depth = 24;
    static constexpr unsigned bits_per_pixel = 32;
    static constexpr unsigned num_colors = 1 << depth;

    image(unsigned w, unsigned h) : image{w, h, 0, nullptr} { }

    image(unsigned w, unsigned h, std::size_t stride, void* p) :
        image_base{image_ptr{pixman_image_create_bits(PIXMAN_x8r8g8b8, w, h, static_cast<uint32_t*>(p), stride)}}
    { }

    ////////////////////
    void fill(int x, int y, unsigned w, unsigned h, const color& c)
    {
        pixman_rectangle16 rect(x, y, w, h);
        pixman_image_fill_rectangles(PIXMAN_OP_SRC, &*pix_, &c, 1, &rect);
    }

    void fill(int x, int y, const image& src)
    {
        pixman_image_composite32(PIXMAN_OP_SRC, &*src.pix_, nullptr, &*pix_, 0, 0, 0, 0, x, y, src.width(), src.height());
    }

    void fill(int x, int y, const image& src, int src_x, int src_y, unsigned w, unsigned h)
    {
        pixman_image_composite32(PIXMAN_OP_SRC, &*src.pix_, nullptr, &*pix_, src_x, src_y, 0, 0, x, y, w, h);
    }

    void alpha_blend(int x, int y, const gray& mask, const color& c)
    {
        image_ptr solid{pixman_image_create_solid_fill(&c)};
        pixman_image_composite32(PIXMAN_OP_OVER, &*solid, &*mask.pix_, &*pix_, 0, 0, 0, 0, x, y, mask.width(), mask.height());
    }
};

////////////////////////////////////////////////////////////////////////////////
}
