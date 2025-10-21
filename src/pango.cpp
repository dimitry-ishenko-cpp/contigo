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

auto create_font_desc(std::string_view font)
{
    pango_font_desc desc{pango_font_description_from_string(font.data()), &pango_font_description_free};
    if (!desc) throw std::runtime_error{"Failed to create font description"};
    return desc;
}

using pango_font = std::unique_ptr<PangoFont, void(*)(void*)>;
using pango_font_metrics = std::unique_ptr<PangoFontMetrics, void(*)(PangoFontMetrics*)>;

auto get_dim_cell(pango_font_map& font_map, pango_context& context, pango_font_desc& font_desc)
{
    pango_font font{pango_font_map_load_font(&*font_map, &*context, &*font_desc), &g_object_unref};
    if (!font) throw std::runtime_error{"Failed to load font"};

    pango_font_metrics metrics{pango_font_get_metrics(&*font, nullptr), &pango_font_metrics_unref};
    if (!metrics) throw std::runtime_error{"Failed to get font metrics"};

    unsigned width = pango_pixels(pango_font_metrics_get_approximate_char_width(&*metrics));
    unsigned height = pango_pixels(pango_font_metrics_get_height(&*metrics));

    return dim{width, height};
}

////////////////////////////////////////////////////////////////////////////////
using pango_layout = std::unique_ptr<PangoLayout, void(*)(void*)>;
using pango_attrs = std::unique_ptr<PangoAttrList, void(*)(PangoAttrList*)>;

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

auto new_attr_bold(bool bold)
{
    return pango_attr_weight_new(bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
}
auto new_attr_italic(bool italic)
{
    return pango_attr_style_new(italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
}
auto new_attr_strike(bool strike)
{
    return pango_attr_strikethrough_new(strike);
}
auto new_attr_under(underline under)
{
    return pango_attr_underline_new(
        (under == under_single) ? PANGO_UNDERLINE_SINGLE :
        (under == under_double) ? PANGO_UNDERLINE_DOUBLE :
        (under == under_error ) ? PANGO_UNDERLINE_ERROR  : PANGO_UNDERLINE_NONE
    );
}

////////////////////////////////////////////////////////////////////////////////
template<typename A>
struct attr_state
{
    unsigned loc = 0;
    A val{};
};

void ucs4_to_utf8(char* out, const char32_t* in)
{
    for (; *in; ++in)
    {
        auto cp = *in;
        if (cp <= 0x7f)
        {
            *out++ = cp;
        }
        else if (cp <= 0x7ff)
        {
            *out++ = 0xc0 | (cp >>  6);
            *out++ = 0x80 | (cp & 0x3f);
        }
        else if (cp <= 0xffff)
        {
            *out++ = 0xe0 | ( cp >> 12);
            *out++ = 0x80 | ((cp >>  6) & 0x3f);
            *out++ = 0x80 | ( cp & 0x3f);
        }
        else if (cp <= 0x10ffff)
        {
            *out++ = 0xf0 | ( cp >> 18);
            *out++ = 0x80 | ((cp >> 12) & 0x3f);
            *out++ = 0x80 | ((cp >>  6) & 0x3f);
            *out++ = 0x80 | ( cp & 0x3f);
        }
    }
    *out = '\0';
}

template<typename A>
void maybe_insert(pango_attrs& attrs, attr_state<A>& state, A new_val, unsigned new_col, auto(*new_attr_fn)(A))
{
    if (state.val != new_val)
    {
        if (state.val != A{})
        {
            auto attr = new_attr_fn(state.val);
            attr->start_index = state.loc;
            attr->end_index = new_col;
            pango_attr_list_insert(&*attrs, attr);
        }
        state.val = new_val;
        state.loc = new_col;
    }
}

void render_background(image<color>& line, std::span<const cell> cells, unsigned cell_width)
{
    unsigned x = 0;
    attr_state<color> bg;

    for (auto&& cell : cells)
    {
        if (x == 0)
            bg.val = cell.bg;
        else if (cell.bg != bg.val)
        {
            fill(line, pos(bg.loc, 0), dim{x - bg.loc, line.height()}, bg.val);
            bg.loc = x;
            bg.val = cell.bg;
        }
        x += cell.width * cell_width;
    }
}

void render_text(image<color>& line, pango_context& context, pango_font_desc& font_desc, std::span<const cell> cells, int x, color fg)
{
    auto layout = create_layout(context, font_desc);
    auto base = pango_pixels(pango_layout_get_baseline(&*layout));

    std::string text;
    auto attrs = create_attrs();

    attr_state<bool> bold, italic, strike;
    attr_state<underline> under;

    unsigned col = 0;
    for (auto&& cell : cells)
    {
        char chars[cell::max_chars];
        ucs4_to_utf8(chars, cell.chars);
        text += chars;

        maybe_insert(attrs, bold, cell.bold, col, &new_attr_bold);
        maybe_insert(attrs, italic, cell.italic, col, &new_attr_italic);
        maybe_insert(attrs, strike, cell.strike, col, &new_attr_strike);
        maybe_insert(attrs, under, cell.under, col, &new_attr_under);

        ++col;
    }

    maybe_insert(attrs, bold, false, col, &new_attr_bold);
    maybe_insert(attrs, italic, false, col, &new_attr_italic);
    maybe_insert(attrs, strike, false, col, &new_attr_strike);
    maybe_insert(attrs, under, under_none, col, &new_attr_under);

    pango_layout_set_text(&*layout, text.data(), -1);
    pango_layout_set_attributes(&*layout, &*attrs);
    auto line0 = pango_layout_get_line_readonly(&*layout, 0);

    image<shade> mask{dim{line.width() - x, line.height()}};

    FT_Bitmap ft_mask;
    ft_mask.rows = mask.height();
    ft_mask.width = mask.width();
    ft_mask.pitch = mask.width() * sizeof(shade);
    ft_mask.buffer = mask.data();
    ft_mask.num_grays = num_colors<shade>;
    ft_mask.pixel_mode = FT_PIXEL_MODE_GRAY;

    pango_ft2_render_layout_line(&ft_mask, line0, 0, base);

    alpha_blend(line, pos{x, 0}, mask, fg);
}

////////////////////////////////////////////////////////////////////////////////
inline auto string(PangoStyle style)
{
    switch (style)
    {
        case PANGO_STYLE_NORMAL : return "normal";
        case PANGO_STYLE_OBLIQUE: return "oblique";
        case PANGO_STYLE_ITALIC : return "italic";
        default: return "???";
    }
}

inline auto string(PangoWeight weight)
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
pango::pango(std::string_view font, dim res, int dpi) : ft_lib_{create_ft_lib()},
    font_map_{create_font_map(dpi)}, context_{create_context(font_map_)}, font_desc_{create_font_desc(font)},
    res_{res}, cell_{get_dim_cell(font_map_, context_, font_desc_)}
{
    auto name = pango_font_description_get_family(&*font_desc_);
    auto style = string(pango_font_description_get_style(&*font_desc_));
    auto weight = string(pango_font_description_get_weight(&*font_desc_));
    auto size = pango_pixels(pango_font_description_get_size(&*font_desc_));

    info() << "Using font: " << name << ", " << style << ", " << weight << ", size: " << size << ", cell: " << cell_.width << "x" << cell_.height;
}

image<color> pango::render(std::span<const cell> cells)
{
    image<color> line{dim{res_.width, cell_.height}};

    render_background(line, cells, cell_.width);

    unsigned x = 0;
    attr_state<color> fg;
    attr_state<unsigned> len;

    for (auto&& cell : cells)
    {
        if (x == 0)
            fg.val = cell.fg;
        else if (cell.fg != fg.val)
        {
            render_text(line, context_, font_desc_, cells.subspan(len.loc, len.val), fg.loc, fg.val);

            fg.loc = x;
            fg.val = cell.fg;

            len.loc += len.val;
            len.val = 0;
        }
        x += cell.width + cell_.width;
        ++len.val;
    }

    return line;
}
