////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "pango.hpp"

#include <stdexcept>
#include <string>

#include <pango/pangoft2.h>

#define pango_pixels PANGO_PIXELS

////////////////////////////////////////////////////////////////////////////////
namespace
{

////////////////////////////////////////////////////////////////////////////////
auto create_ft_lib()
{
    FT_Library lib;
    auto code = FT_Init_FreeType(&lib);
    if (code) throw std::runtime_error{"Failed to init freetype2"};
    return ft_lib{lib, &FT_Done_FreeType};
}

auto create_font_map(int dpi)
{
    pango_font_map font_map{pango_ft2_font_map_new(), &g_object_unref};
    if (!font_map) throw std::runtime_error{"Failed to create fontmap"};
    pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(&*font_map), dpi, dpi);
    return font_map;
}

auto create_context(pango_font_map& fontmap)
{
    pango_context context{pango_font_map_create_context(&*fontmap), &g_object_unref};
    if (!context) throw std::runtime_error{"Failed to create pango context"};
    return context;
}

auto create_font_desc(std::string_view font_desc)
{
    pango_font_desc desc{pango_font_description_from_string(font_desc.data()), &pango_font_description_free};
    if (!desc) throw std::runtime_error{"Failed to create font description"};
    return desc;
}

auto get_metrics(pango_font_map& font_map, pango_context& context, pango_font_desc& font_desc)
{
    pango_font font{pango_font_map_load_font(&*font_map, &*context, &*font_desc), &g_object_unref};
    if (!font) throw std::runtime_error{"Failed to load font"};

    pango_font_metrics metrics{pango_font_get_metrics(&*font, nullptr), &pango_font_metrics_unref};
    if (!metrics) throw std::runtime_error{"Failed to get font metrics"};

    return metrics;
}

auto create_layout(pango_context& context, pango_font_desc& font_desc)
{
    pango_layout layout{pango_layout_new(&*context), &g_object_unref};
    pango_layout_set_font_description(&*layout, &*font_desc);
    return layout;
}

auto create_attrs()
{
    pango_attrs attrs{pango_attr_list_new(), &pango_attr_list_unref};
    if (!attrs) throw std::runtime_error{"Failed to create attribute list"};
    return attrs;
}

void maybe_insert_bold(pango_attrs& attrs, unsigned from, unsigned to, bool bold)
{
    if (bold)
    {
        auto attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_italic(pango_attrs& attrs, unsigned from, unsigned to, bool italic)
{
    if (italic)
    {
        auto attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_strike(pango_attrs& attrs, unsigned from, unsigned to, bool strike)
{
    if (strike)
    {
        auto attr = pango_attr_strikethrough_new(true);
        attr->start_index = from;
        attr->end_index = to;
        pango_attr_list_insert(&*attrs, attr);
    }
}

void maybe_insert_under(pango_attrs& attrs, unsigned from, unsigned to, unsigned underline)
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

////////////////////////////////////////////////////////////////////////////////
auto string(PangoStyle style)
{
    switch (style)
    {
        case PANGO_STYLE_NORMAL : return "normal";
        case PANGO_STYLE_OBLIQUE: return "oblique";
        case PANGO_STYLE_ITALIC : return "italic";
        default: return "???";
    }
}

auto string(PangoWeight weight)
{
    switch (weight)
    {
        case PANGO_WEIGHT_THIN: return "thin";
        case PANGO_WEIGHT_ULTRALIGHT: return "ultra-light";
        case PANGO_WEIGHT_LIGHT: return "light";
        case PANGO_WEIGHT_SEMILIGHT: return "semi-light";
        case PANGO_WEIGHT_BOOK: return "book";
        case PANGO_WEIGHT_NORMAL: return "normal";
        case PANGO_WEIGHT_MEDIUM: return "medium";
        case PANGO_WEIGHT_SEMIBOLD: return "semi-bold";
        case PANGO_WEIGHT_BOLD: return "bold";
        case PANGO_WEIGHT_ULTRABOLD: return "";
        case PANGO_WEIGHT_HEAVY: return "heavy";
        case PANGO_WEIGHT_ULTRAHEAVY: return "ultra-heavy";
        default: return "???";
    }
}

}

////////////////////////////////////////////////////////////////////////////////
pango::pango(std::string_view font_desc, dim mode, unsigned dpi) : ft_lib_{create_ft_lib()},
    font_map_{create_font_map(dpi)}, context_{create_context(font_map_)}, font_desc_{create_font_desc(font_desc)},
    mode_{mode},
    layout_{create_layout(context_, font_desc_)}
{
    auto metrics = get_metrics(font_map_, context_, font_desc_);
    cell_.width = pango_pixels(pango_font_metrics_get_approximate_char_width(&*metrics));
    cell_.height = pango_pixels(pango_font_metrics_get_height(&*metrics));

    baseline_ = pango_pixels(pango_layout_get_baseline(&*layout_));

    ////////////////////
    auto name = pango_font_description_get_family(&*font_desc_);
    auto style= string(pango_font_description_get_style(&*font_desc_));
    auto wght = string(pango_font_description_get_weight(&*font_desc_));
    auto size = pango_pixels(pango_font_description_get_size(&*font_desc_));

    info() << "Using font: " << name << ", style: " << style << ", weight: " << wght << ", size=" << size << ", cell=" << cell_;
}

////////////////////////////////////////////////////////////////////////////////
void pango::render_text(image<color>& image_line, pos pos, dim dim, std::span<const cell> cells, color fg)
{
    auto attrs = create_attrs();

    auto begin = cells.begin();
    auto from_bold = begin, from_italic = begin, from_strike = begin, from_under = begin;
    std::string text = begin->chars;

    for (auto to = begin + 1; to != cells.end(); ++to)
    {
        text += to->chars;
        if (to->bold != from_bold->bold)
        {
            maybe_insert_bold(attrs, from_bold - begin, to - begin, from_bold->bold);
            from_bold = to;
        }
        if (to->italic != from_italic->italic)
        {
            maybe_insert_italic(attrs, from_italic - begin, to - begin, from_italic->italic);
            from_italic = to;
        }
        if (to->strike != from_strike->strike)
        {
            maybe_insert_strike(attrs, from_strike - begin, to - begin, from_strike->strike);
            from_strike = to;
        }
        if (to->underline != from_under->underline)
        {
            maybe_insert_under(attrs, from_under - begin, to - begin, from_under->underline);
            from_under = to;
        }
    }

    maybe_insert_bold(attrs, from_bold - begin, cells.size(), from_bold->bold);
    maybe_insert_italic(attrs, from_italic - begin, cells.size(), from_italic->italic);
    maybe_insert_strike(attrs, from_strike - begin, cells.size(), from_strike->strike);
    maybe_insert_under(attrs, from_under - begin, cells.size(), from_under->underline);

    pango_layout_set_text(&*layout_, text.data(), -1);
    pango_layout_set_attributes(&*layout_, &*attrs);
    auto line = pango_layout_get_line_readonly(&*layout_, 0);

    image<shade> mask{dim, 0};
    FT_Bitmap ft_mask;
    ft_mask.rows = mask.height();
    ft_mask.width = mask.width();
    ft_mask.pitch = mask.stride();
    ft_mask.buffer = mask.data();
    ft_mask.num_grays = num_colors<shade>;
    ft_mask.pixel_mode = FT_PIXEL_MODE_GRAY;
    pango_ft2_render_layout_line(&ft_mask, line, 0, baseline_);

    alpha_blend(image_line, pos, mask, fg);
}

image<color> pango::render(std::span<const cell> cells)
{
    image<color> image_line{dim{mode_.width, cell_.height}};

    pos pos{0, 0};
    dim dim{cell_.width, image_line.height()};

    // render background
    auto from = cells.begin();
    for (auto to = from + 1; to != cells.end(); ++to, dim.width += cell_.width)
    {
        if (to->bg != from->bg)
        {
            fill(image_line, pos, dim, from->bg);

            from = to;
            pos.x += dim.width;
            dim.width = 0;
        }
    }
    fill(image_line, pos, dim, from->bg);

    ////////////////////
    pos.x = 0;
    dim.width = cell_.width;

    from = cells.begin();
    for (auto to = from + 1; to != cells.end(); ++to, dim.width += cell_.width)
    {
        if (to->fg != from->fg)
        {
            render_text(image_line, pos, dim, std::span{from, to}, from->fg);
            from = to;
            pos.x += dim.width;
            dim.width = 0;
        }
    }
    render_text(image_line, pos, dim, std::span{from, cells.end()}, from->fg);

    return image_line;
}
