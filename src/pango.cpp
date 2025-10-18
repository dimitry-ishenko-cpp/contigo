////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "pango.hpp"

#include <stdexcept>
#include <pango/pangoft2.h>

#define pango_pixels PANGO_PIXELS

////////////////////////////////////////////////////////////////////////////////
namespace
{

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

auto get_metrics(pango_font_map& fontmap, pango_context& context, pango_font_desc& font_desc)
{
    pango_font font{pango_font_map_load_font(&*fontmap, &*context, &*font_desc), &g_object_unref};
    if (!font) throw std::runtime_error{"Failed to load font"};

    pango_font_metrics metrics{pango_font_get_metrics(&*font, nullptr), &pango_font_metrics_unref};
    if (!metrics) throw std::runtime_error{"Failed to get font metrics"};

    unsigned width = pango_pixels(pango_font_metrics_get_approximate_char_width(&*metrics));
    unsigned height = pango_pixels(pango_font_metrics_get_height(&*metrics));

    return dim{width, height};
}

auto string(PangoStyle style)
{
    switch (style)
    {
    case PANGO_STYLE_NORMAL: return "normal";
    case PANGO_STYLE_OBLIQUE: return "oblique";
    case PANGO_STYLE_ITALIC: return "italic";
    default: return "???";
    }
}

////////////////////////////////////////////////////////////////////////////////
pango::pango(std::string_view font, int dpi) : ft_lib_{create_ft_lib()},
    font_map_{create_font_map(dpi)}, context_{create_context(font_map_)}, font_desc_{create_font_desc(font)},
    dim_cell_{get_metrics(font_map_, context_, font_desc_)}
{
    auto desc = &*font_desc_;
    auto name = pango_font_description_get_family(desc);
    auto style= string(pango_font_description_get_style(desc));
    auto wght = pango_font_description_get_weight(desc);
    auto size = pango_pixels(pango_font_description_get_size(desc));

    info() << "Using font: " << name << ", " << style << ", " << wght << ", size: " << size << ", cell: " << dim_cell_.width << "x" << dim_cell_.height;
}

bitmap<xrgb> pango::render_row(std::span<const cell> cells)
{
    // TODO
    return bitmap<xrgb>{dim{0, 0}};
}
