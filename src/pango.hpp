////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "bitmap.hpp"
#include "cell.hpp"
#include "color.hpp"
#include "geom.hpp"

#include <memory>
#include <span>
#include <string_view>

struct FT_LibraryRec_;
using ft_lib = std::unique_ptr<FT_LibraryRec_, int(*)(FT_LibraryRec_*)>;

struct _PangoFontMap;
using pango_fontmap = std::unique_ptr<_PangoFontMap, void(*)(void*)>;

struct _PangoContext;
using pango_context = std::unique_ptr<_PangoContext, void(*)(void*)>;

struct _PangoFontDescription;
using pango_font_desc = std::unique_ptr<_PangoFontDescription, void(*)(_PangoFontDescription*)>;

////////////////////////////////////////////////////////////////////////////////
class pango
{
public:
    ////////////////////
    pango(std::string_view font, int dpi);

    constexpr auto cell_dim() const noexcept { return cell_dim_; }

    bitmap<color> render(std::span<cell>);

private:
    ////////////////////
    ft_lib ft_lib_;
    pango_fontmap fontmap_;
    pango_context context_;
    pango_font_desc font_desc_;

    dim cell_dim_;
};
