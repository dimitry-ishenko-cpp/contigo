////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "pixman.hpp"

#include <memory>
#include <span>
#include <string_view>

#include <pango/pangoft2.h>

namespace vte { struct cell; }

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

struct cell
{
    unsigned width, height;
    int baseline;
};

////////////////////////////////////////////////////////////////////////////////
class engine
{
public:
    ////////////////////
    engine(std::string_view font_desc, unsigned dpi);

    constexpr auto& cell() const noexcept { return cell_; }

    pixman::image render_line(unsigned width, std::span<const vte::cell>);

private:
    ////////////////////
    ft_lib_ptr ft_lib_;
    font_map_ptr font_map_;
    context_ptr context_;
    font_desc_ptr font_desc_;

    layout_ptr layout_;
    pango::cell cell_;

    void render_chunk(pixman::image&, int x, int y, unsigned w, unsigned h, std::span<const vte::cell>, const pixman::color&);
};

////////////////////////////////////////////////////////////////////////////////
}
