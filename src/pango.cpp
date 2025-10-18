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

auto create_pango_fontmap(int dpi)
{
    pango_fontmap fontmap{pango_ft2_font_map_new(), &g_object_unref};
    if (!fontmap) throw std::runtime_error{"Failed to create fontmap"};
    pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(&*fontmap), dpi, dpi);
    return fontmap;
}

auto create_pango_context(pango_fontmap& fontmap)
{
    pango_context context{pango_font_map_create_context(&*fontmap), &g_object_unref};
    if (!context) throw std::runtime_error{"Failed to create pango context"};
    return context;
}

auto create_pango_font_desc(std::string_view font)
{
    pango_font_desc desc{pango_font_description_from_string(font.data()), &pango_font_description_free};
    if (!desc) throw std::runtime_error{"Failed to create font description"};
    return desc;
}

using pango_font = std::unique_ptr<PangoFont, void(*)(void*)>;
using pango_font_metrics = std::unique_ptr<PangoFontMetrics, void(*)(PangoFontMetrics*)>;

}

////////////////////////////////////////////////////////////////////////////////
pango::pango(std::string_view font, int dpi) :
    ft_lib_{create_ft_lib()},
    fontmap_{create_pango_fontmap(dpi)},
    context_{create_pango_context(fontmap_)},
    font_desc_{create_pango_font_desc(font)}
{
    pango_font font_{pango_font_map_load_font(&*fontmap_, &*context_, &*font_desc_), &g_object_unref};
    if (!font_) throw std::runtime_error{"Failed to load font"};

    pango_font_metrics metrics_{pango_font_get_metrics(&*font_, nullptr), &pango_font_metrics_unref};
    if (!metrics_) throw std::runtime_error{"Failed to get font metrics"};

    cell_dim_.width = PANGO_PIXELS(pango_font_metrics_get_approximate_char_width(&*metrics_));
    cell_dim_.height = PANGO_PIXELS(pango_font_metrics_get_height(&*metrics_));

    auto name = pango_font_description_get_family(&*font_desc_);
    auto size = PANGO_PIXELS(pango_font_description_get_size(&*font_desc_));

    info() << "Using font: " << name << ", size: " << size << " pt, cell: " << cell_dim_.width << "x" << cell_dim_.height << " px";
}

bitmap<xrgb> pango::render(std::span<const cell> cells)
{
    // TODO
    return bitmap<xrgb>{dim{0, 0}};
}
