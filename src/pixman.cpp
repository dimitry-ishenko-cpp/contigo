////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "pixman.hpp"

////////////////////////////////////////////////////////////////////////////////
namespace pixman
{

void image_delete::operator()(pixman_image* img) { pixman_image_unref(img); }

////////////////////////////////////////////////////////////////////////////////
unsigned image_base::width() const { return pixman_image_get_width(&*img_); }
unsigned image_base::height() const { return pixman_image_get_height(&*img_); }

std::size_t image_base::stride() const { return pixman_image_get_stride(&*img_); }

void* image_base::data() { return pixman_image_get_data(&*img_); }

////////////////////////////////////////////////////////////////////////////////
gray::gray(unsigned w, unsigned h) :
    image_base{image_ptr{pixman_image_create_bits(PIXMAN_a8, w, h, nullptr, 0)}}
{ }

solid::solid(const color& c) :
    image_base{image_ptr{pixman_image_create_solid_fill(&c)}}
{ }

////////////////////////////////////////////////////////////////////////////////
image::image(unsigned w, unsigned h) : image{w, h, 0, nullptr} { }

image::image(unsigned w, unsigned h, std::size_t stride, void* p) :
    image_base{image_ptr{pixman_image_create_bits(PIXMAN_x8r8g8b8, w, h, static_cast<std::uint32_t*>(p), stride)}}
{ }

void image::fill(int x, int y, unsigned w, unsigned h, const color& bg)
{
    pixman_rectangle16 rect(x, y, w, h);
    pixman_image_fill_rectangles(PIXMAN_OP_SRC, &*img_, &bg, 1, &rect);
}

void image::fill(int x, int y, const image& image)
{
    pixman_image_composite(PIXMAN_OP_SRC, &*image.img_, nullptr, &*img_, 0, 0, 0, 0, x, y, image.width(), image.height());
}

void image::alpha_blend(int x, int y, const gray& mask, const color& fg)
{
    solid solid{fg};
    pixman_image_composite(PIXMAN_OP_OVER, &*solid.img_, &*mask.img_, &*img_, 0, 0, 0, 0, x, y, mask.width(), mask.height());
}

}
