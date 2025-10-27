////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cell.hpp"
#include "pixman.hpp"

#include <memory>
#include <span>
#include <string_view>

#include <pango/pangoft2.h>

////////////////////////////////////////////////////////////////////////////////
namespace pango
{

using ft_lib_ptr = std::unique_ptr<FT_LibraryRec_, int(*)(FT_LibraryRec_*)>;
using font_map_ptr = std::unique_ptr<PangoFontMap, void(*)(void*)>;
using context_ptr = std::unique_ptr<PangoContext, void(*)(void*)>;
using font_desc_ptr = std::unique_ptr<PangoFontDescription, void(*)(PangoFontDescription*)>;
using font_ptr = std::unique_ptr<PangoFont, void(*)(void*)>;
using font_metrics_ptr = std::unique_ptr<PangoFontMetrics, void(*)(PangoFontMetrics*)>;
using layout_ptr = std::unique_ptr<PangoLayout, void(*)(void*)>;
using attrs_ptr = std::unique_ptr<PangoAttrList, void(*)(PangoAttrList*)>;

using pixman::color;
constexpr bool operator==(const color& x, const color& y) noexcept
{
    return x.red == y.red && x.green == y.green && x.blue == y.blue && x.alpha == y.alpha;
}

////////////////////////////////////////////////////////////////////////////////
class engine
{
public:
    ////////////////////
    engine(std::string_view font_desc, unsigned width, unsigned dpi);

    constexpr auto cell_width() const noexcept { return cell_width_; }
    constexpr auto cell_height() const noexcept { return cell_height_; }

    pixman::image render_line(std::span<const cell>);

private:
    ////////////////////
    ft_lib_ptr ft_lib_;
    font_map_ptr font_map_;
    context_ptr context_;
    font_desc_ptr font_desc_;

    unsigned width_;
    unsigned cell_width_, cell_height_;

    layout_ptr layout_;
    int baseline_;

    void render_text(pixman::image&, int x, int y, unsigned w, unsigned h, std::span<const cell>, const color&);
};

}
