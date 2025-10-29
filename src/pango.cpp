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
#include <string>

#define pango_pixels PANGO_PIXELS

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

auto get_metrics(font_map_ptr& font_map, context_ptr& context, font_desc_ptr& font_desc)
{
    font_ptr font{pango_font_map_load_font(&*font_map, &*context, &*font_desc), &g_object_unref};
    if (!font) throw std::runtime_error{"Failed to load font"};

    font_metrics_ptr metrics{pango_font_get_metrics(&*font, nullptr), &pango_font_metrics_unref};
    if (!metrics) throw std::runtime_error{"Failed to get font metrics"};

    return metrics;
}

auto create_layout(context_ptr& context, font_desc_ptr& font_desc)
{
    layout_ptr layout{pango_layout_new(&*context), &g_object_unref};
    pango_layout_set_font_description(&*layout, &*font_desc);
    return layout;
}

auto create_attrs()
{
    attrs_ptr attrs{pango_attr_list_new(), &pango_attr_list_unref};
    if (!attrs) throw std::runtime_error{"Failed to create attribute list"};
    return attrs;
}

void maybe_insert_bold(attrs_ptr& attrs, unsigned from, unsigned to, bool bold)
{
    if (bold)
    {
        auto attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_italic(attrs_ptr& attrs, unsigned from, unsigned to, bool italic)
{
    if (italic)
    {
        auto attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_strike(attrs_ptr& attrs, unsigned from, unsigned to, bool strike)
{
    if (strike)
    {
        auto attr = pango_attr_strikethrough_new(true);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_underline(attrs_ptr& attrs, unsigned from, unsigned to, unsigned underline)
{
    PangoUnderline val;
    switch (underline)
    {
        case 1: val = PANGO_UNDERLINE_SINGLE; break;
        case 2: val = PANGO_UNDERLINE_DOUBLE; break;
        case 3: val = PANGO_UNDERLINE_ERROR ; break;
        default: return;
    }

    auto attr = pango_attr_underline_new(val);
    attr->start_index = from;
    attr->end_index = to;
    pango_attr_list_insert(&*attrs, attr);
}

}

////////////////////////////////////////////////////////////////////////////////
engine::engine(std::string_view font_desc, unsigned dpi) :
    ft_lib_{create_ft_lib()},
    font_map_{create_font_map(dpi)}, context_{create_context(font_map_)}, font_desc_{create_font_desc(font_desc)},
    layout_{create_layout(context_, font_desc_)}
{
    auto metrics = get_metrics(font_map_, context_, font_desc_);
    cell_width_ = pango_pixels(pango_font_metrics_get_approximate_char_width(&*metrics));
    cell_height_ = pango_pixels(pango_font_metrics_get_height(&*metrics));

    baseline_ = pango_pixels(pango_layout_get_baseline(&*layout_));

    ////////////////////
    auto name = pango_font_description_get_family(&*font_desc_);
    auto style = pango_font_description_get_style(&*font_desc_);
    auto weight = pango_font_description_get_weight(&*font_desc_);
    auto size = pango_pixels(pango_font_description_get_size(&*font_desc_));

    info() << "Using font: " << name << ", style=" << style << ", weight=" << weight << ", size=" << size << ", cell=" << cell_width_ << "x" << cell_height_;
}

constexpr bool operator==(const color& x, const color& y) noexcept
{
    return x.red == y.red && x.green == y.green && x.blue == y.blue && x.alpha == y.alpha;
}

void engine::render_chunk(pixman::image& image, int x, int y, unsigned w, unsigned h, std::span<const vte::cell> cells, const color& fg)
{
    auto attrs = create_attrs();

    auto cell = cells.begin();
    auto bold = cell->bold, italic = cell->italic, strike = cell->strike;
    auto underline = cell->underline;
    unsigned pos_bold = 0, pos_italic = 0, pos_strike = 0, pos_underline = 0;

    std::string chunk;
    unsigned pos = 0;

    for (; cell != cells.end(); ++cell)
    {
        std::string chars{cell->chars[0] ? cell->chars : " "};
        chunk += chars;

        if (cell->bold != bold)
        {
            maybe_insert_bold(attrs, pos_bold, pos, bold);
            bold = cell->bold;
            pos_bold = pos;
        }
        if (cell->italic != italic)
        {
            maybe_insert_italic(attrs, pos_italic, pos, italic);
            italic = cell->italic;
            pos_italic = pos;
        }
        if (cell->strike != strike)
        {
            maybe_insert_strike(attrs, pos_strike, pos, strike);
            strike = cell->strike;
            pos_strike = pos;
        }
        if (cell->underline != underline)
        {
            maybe_insert_underline(attrs, pos_underline, pos, underline);
            underline = cell->underline;
            pos_underline = pos;
        }

        pos += chars.size();
    }

    maybe_insert_bold(attrs, pos_bold, pos, bold);
    maybe_insert_italic(attrs, pos_italic, pos, italic);
    maybe_insert_strike(attrs, pos_strike, pos, strike);
    maybe_insert_underline(attrs, pos_underline, pos, underline);

    pango_layout_set_text(&*layout_, chunk.data(), chunk.size());
    pango_layout_set_attributes(&*layout_, &*attrs);
    auto line = pango_layout_get_line_readonly(&*layout_, 0);

    pixman::gray mask{w, h};
    FT_Bitmap ftb;
    ftb.rows  = mask.height();
    ftb.width = mask.width();
    ftb.pitch = mask.stride();
    ftb.buffer= mask.data<uint8_t*>();
    ftb.num_grays = mask.num_colors;
    ftb.pixel_mode = FT_PIXEL_MODE_GRAY;
    pango_ft2_render_layout_line(&ftb, line, 0, baseline_);

    image.alpha_blend(x, y, mask, fg);
}

pixman::image engine::render_line(unsigned width, std::span<const vte::cell> cells)
{
    pixman::image image{width, cell_height_};

    // render background
    int x = 0, y = 0;
    unsigned w = 0, h = cell_height_;

    auto from = cells.begin();
    for (auto to = from; to != cells.end(); ++to, w += cell_width_)
    {
        if (to->bg != from->bg)
        {
            image.fill(x, y, w, h, from->bg);

            from = to;
            x += w; w = 0;
        }
    }
    image.fill(x, y, w, h, from->bg);

    // render text
    x = 0; w = 0;

    from = cells.begin();
    for (auto to = from; to != cells.end(); ++to, w += cell_width_)
    {
        if (to->fg != from->fg)
        {
            render_chunk(image, x, y, w, h, std::span{from, to}, from->fg);
            from = to;
            x += w; w = 0;
        }
    }
    render_chunk(image, x, y, w, h, std::span{from, cells.end()}, from->fg);

    return image;
}

////////////////////////////////////////////////////////////////////////////////
}
