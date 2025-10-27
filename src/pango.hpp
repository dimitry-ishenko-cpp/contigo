////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cell.hpp"
#include "geom.hpp"
#include "pixman.hpp"

#include <memory>
#include <span>
#include <string_view>

struct FT_LibraryRec_;
using ft_lib = std::unique_ptr<FT_LibraryRec_, int(*)(FT_LibraryRec_*)>;

struct _PangoFontMap;
using pango_font_map = std::unique_ptr<_PangoFontMap, void(*)(void*)>;

struct _PangoContext;
using pango_context = std::unique_ptr<_PangoContext, void(*)(void*)>;

struct _PangoFontDescription;
using pango_font_desc = std::unique_ptr<_PangoFontDescription, void(*)(_PangoFontDescription*)>;

struct _PangoFont;
using pango_font = std::unique_ptr<_PangoFont, void(*)(void*)>;

struct _PangoFontMetrics;
using pango_font_metrics = std::unique_ptr<_PangoFontMetrics, void(*)(_PangoFontMetrics*)>;

struct _PangoLayout;
using pango_layout = std::unique_ptr<_PangoLayout, void(*)(void*)>;

struct _PangoAttrList;
using pango_attrs = std::unique_ptr<_PangoAttrList, void(*)(_PangoAttrList*)>;

////////////////////////////////////////////////////////////////////////////////
class pango
{
public:
    ////////////////////
    pango(std::string_view font_desc, unsigned width, unsigned dpi);

    constexpr auto dim_cell() const noexcept { return cell_; }

    pixman::image render_line(std::span<const cell>);

private:
    ////////////////////
    ft_lib ft_lib_;
    pango_font_map font_map_;
    pango_context context_;
    pango_font_desc font_desc_;

    unsigned width_;
    dim cell_;

    pango_layout layout_;
    int baseline_;

    void render_text(pixman::image&, pos, dim, std::span<const cell>, xrgb32);
};
