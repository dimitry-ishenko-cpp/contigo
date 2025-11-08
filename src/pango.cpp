////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "pango.hpp"
#include "vte.hpp"

#include <stdexcept>

#define pango_pixels PANGO_PIXELS_CEIL

////////////////////////////////////////////////////////////////////////////////
namespace pango
{

namespace
{

auto create_ft_lib()
{
    FT_Library lib;
    auto code = FT_Init_FreeType(&lib);
    if (code) throw std::runtime_error{"Failed to init freetype2"};
    return ft_lib_ptr{lib, &FT_Done_FreeType};
}

auto create_font_map(int dpi)
{
    font_map_ptr font_map{pango_ft2_font_map_new(), &g_object_unref};
    if (!font_map) throw std::runtime_error{"Failed to create fontmap"};
    pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(&*font_map), dpi, dpi);
    return font_map;
}

auto create_context(font_map_ptr& fontmap)
{
    context_ptr context{pango_font_map_create_context(&*fontmap), &g_object_unref};
    if (!context) throw std::runtime_error{"Failed to create pango context"};
    return context;
}

auto create_font_desc(std::string_view font_desc)
{
    font_desc_ptr desc{pango_font_description_from_string(font_desc.data()), &pango_font_description_free};
    if (!desc) throw std::runtime_error{"Failed to create font description"};
    return desc;
}

auto get_box(font_map_ptr& font_map, context_ptr& context, font_desc_ptr& font_desc, layout_ptr& layout)
{
    font_ptr font{pango_font_map_load_font(&*font_map, &*context, &*font_desc), &g_object_unref};
    if (!font) throw std::runtime_error{"Failed to load font"};

    font_metrics_ptr metrics{pango_font_get_metrics(&*font, nullptr), &pango_font_metrics_unref};
    if (!metrics) throw std::runtime_error{"Failed to get font metrics"};

    return pango::box{
        .width = pango_pixels(pango_font_metrics_get_approximate_char_width(&*metrics)),
        .height = pango_pixels(pango_font_metrics_get_height(&*metrics)),
        .baseline = pango_pixels(pango_layout_get_baseline(&*layout))
    };
}

auto create_layout(context_ptr& context, font_desc_ptr& font_desc)
{
    layout_ptr layout{pango_layout_new(&*context), &g_object_unref};
    pango_layout_set_font_description(&*layout, &*font_desc);
    return layout;
}

constexpr PangoUnderline to_pango[] =
{
    PANGO_UNDERLINE_NONE,
    PANGO_UNDERLINE_SINGLE,
    PANGO_UNDERLINE_DOUBLE,
    PANGO_UNDERLINE_ERROR,
};

auto create_attrs(const vte::cell& cell)
{
    attrs_ptr attrs{pango_attr_list_new(), &pango_attr_list_unref};
    if (!attrs) throw std::runtime_error{"Failed to create attribute list"};

    if (cell.attrs.bold)
    {
        auto attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = 0;
        attr->end_index = cell.max_chars;
        pango_attr_list_insert(&*attrs, attr);
    }

    if (cell.attrs.italic)
    {
        auto attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
        attr->start_index = 0;
        attr->end_index = cell.max_chars;
        pango_attr_list_insert(&*attrs, attr);
    }

    if (cell.attrs.strike)
    {
        auto attr = pango_attr_strikethrough_new(true);
        attr->start_index = 0;
        attr->end_index = cell.max_chars;
        pango_attr_list_insert(&*attrs, attr);
    }

    if (cell.attrs.underline)
    {
        auto attr = pango_attr_underline_new(to_pango[cell.attrs.underline]);
        attr->start_index = 0;
        attr->end_index = cell.max_chars;
        pango_attr_list_insert(&*attrs, attr);
    }

    return attrs;
}

constexpr bool operator==(const pixman::color& x, const pixman::color& y) noexcept
{
    return x.red == y.red && x.green == y.green && x.blue == y.blue && x.alpha == y.alpha;
}

constexpr bool operator==(const vte::attrs& x, const vte::attrs& y) noexcept
{
    return x.bold == y.bold && x.italic == y.italic && x.strike == y.strike && x.underline == y.underline;
}

}

////////////////////////////////////////////////////////////////////////////////
engine::engine(std::string_view font_desc, unsigned dpi) :
    ft_lib_{create_ft_lib()},
    font_map_{create_font_map(dpi)}, context_{create_context(font_map_)}, font_desc_{create_font_desc(font_desc)},
    layout_{create_layout(context_, font_desc_)},
    box_{get_box(font_map_, context_, font_desc_, layout_)}
{
    auto name = pango_font_description_get_family(&*font_desc_);
    auto style = pango_font_description_get_style(&*font_desc_);
    auto weight = pango_font_description_get_weight(&*font_desc_);
    auto size = pango_pixels(pango_font_description_get_size(&*font_desc_));

    info() << "Using font: " << name << ", style=" << style << ", weight=" << weight << ", size=" << size << ", box=" << box_.width << "x" << box_.height;
}

pixman::image engine::render(std::span<const vte::cell> cells)
{
    unsigned w = box_.width * cells.size(), h = box_.height;
    pixman::image image{w, h};

    // render background
    int x = 0, y = 0; w = 0;

    auto from = cells.begin();
    auto fbg = from->bg;

    for (auto to = from; to < cells.end(); to += to->width)
    {
        auto tbg = to->bg;
        if (tbg != fbg)
        {
            image.fill(x, y, w, h, fbg);

            from = to; fbg = tbg;
            x += w; w = 0;
        }

        w += box_.width * to->width;
    }
    image.fill(x, y, w, h, fbg);

    // render text
    x = 0;

    from = cells.begin();
    auto attrs = create_attrs(*from);

    for (auto to = from; to < cells.end(); to += to->width)
    {
        if (to->chars[0] && !to->attrs.conceal && (to->chars[0] != ' ' || to->attrs.reverse))
        {
            if (to->attrs != from->attrs)
            {
                from = to;
                attrs = create_attrs(*from);
            }
            render(image, x, y, *to, attrs);
        }
        x += box_.width * to->width;
    }

    return image;
}

void engine::render(pixman::image& image, int x, int y, const vte::cell& cell, const attrs_ptr& attrs)
{
    pango_layout_set_text(&*layout_, cell.chars, cell.len);
    pango_layout_set_attributes(&*layout_, &*attrs);
    auto symbol = pango_layout_get_line_readonly(&*layout_, 0);

    // +1 to allow overhang on the right
    pixman::gray mask{box_.width * (cell.width + 1), box_.height};
    FT_Bitmap ftb;
    ftb.rows  = mask.height();
    ftb.width = mask.width();
    ftb.pitch = mask.stride();
    ftb.buffer= mask.data<uint8_t*>();
    ftb.num_grays = mask.num_colors;
    ftb.pixel_mode = FT_PIXEL_MODE_GRAY;
    pango_ft2_render_layout_line(&ftb, symbol, 0, box_.baseline);

    image.alpha_blend(x, y, mask, cell.fg);
}

////////////////////////////////////////////////////////////////////////////////
}
