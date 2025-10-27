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

namespace
{

inline auto pix_color(xrgb32 c)
{
    pixman_color_t pc;
    pc.red  = c.r << 8;
    pc.green= c.g << 8;
    pc.blue = c.b << 8;
    pc.alpha= 0xffff;
    return pc;
}

auto create_solid(xrgb32 c)
{
    auto pc = pix_color(c);
    return image_ptr{pixman_image_create_solid_fill(&pc), &pixman_image_unref};
}

auto create_image(pixman_format_code_t format, dim dim, std::size_t stride = 0, void* data = nullptr)
{
    return image_ptr{
        pixman_image_create_bits(format, dim.width, dim.height, static_cast<std::uint32_t*>(data), stride),
        &pixman_image_unref
    };
}

}

////////////////////////////////////////////////////////////////////////////////
unsigned image_base::width() const { return pixman_image_get_width(&*img_); }
unsigned image_base::height() const { return pixman_image_get_height(&*img_); }

std::size_t image_base::stride() const { return pixman_image_get_stride(&*img_); }

void* image_base::data() { return pixman_image_get_data(&*img_); }

////////////////////////////////////////////////////////////////////////////////
gray::gray(struct dim dim) : image_base{create_image(PIXMAN_a8, dim)} { }
solid::solid(xrgb32 c) : image_base{create_solid(c)} { }

////////////////////////////////////////////////////////////////////////////////
image::image(struct dim dim) : image_base{create_image(PIXMAN_x8r8g8b8, dim)} { }

image::image(struct dim dim, std::size_t stride, void* data) :
    image_base{create_image(PIXMAN_x8r8g8b8, dim, stride, data)}
{ }

void image::fill(pos pos, struct dim dim, xrgb32 bg)
{
    auto pc = pix_color(bg);

    pixman_rectangle16_t rect;
    rect.x = pos.x;
    rect.y = pos.y;
    rect.width = dim.width;
    rect.height = dim.height;

    pixman_image_fill_rectangles(PIXMAN_OP_SRC, &*img_, &pc, 1, &rect);
}

void image::fill(pos pos, const image& image)
{
    pixman_image_composite(PIXMAN_OP_SRC, &*image.img_, nullptr, &*img_,
        0, 0, 0, 0, pos.x, pos.y, image.width(), image.height()
    );
}

void image::alpha_blend(pos pos, const gray& mask, xrgb32 fg)
{
    solid solid{fg};
    pixman_image_composite(PIXMAN_OP_OVER, &*solid.img_, &*mask.img_, &*img_,
        0, 0, 0, 0, pos.x, pos.y, mask.width(), mask.height()
    );
}

}
