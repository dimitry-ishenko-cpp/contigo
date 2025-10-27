////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "vte.hpp"

#include <cstring> // std::memcpy
#include <vterm.h>

////////////////////////////////////////////////////////////////////////////////
struct vte::dispatch
{

static int damage(VTermRect rect, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->row_cb_) for (auto row = rect.start_row; row < rect.end_row; ++row) vt->change(row);
    return true;
}

static int move_rect(VTermRect dst, VTermRect src, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->move_cb_)
    {
        int row = src.start_row;
        unsigned rows = src.end_row - src.start_row;
        int distance = dst.start_row - src.start_row;
        vt->move_cb_(row, rows, distance);
    }
    return true;
}

static int move_cursor(VTermPos pos, VTermPos old_pos, int visible, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    // TODO
    return true;
}

static int set_prop(VTermProp prop, VTermValue* val, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    // TODO
    return true;
}

static int bell(void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    // TODO
    return true;
}

static int resize(int rows, int cols, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->size_cb_)
    {
        struct dim dim(cols, rows);
        vt->size_cb_(dim);
    }
    return true;
}

};

////////////////////////////////////////////////////////////////////////////////
namespace
{

void ucs4_to_utf8(char* out, const uint32_t* in)
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

auto to_cell(const VTermScreenCell& vc)
{
    cell cell;

    ucs4_to_utf8(cell.chars, vc.chars);
    cell.width = vc.width;

    cell.bold  = vc.attrs.bold;
    cell.italic= vc.attrs.italic;
    cell.strike= vc.attrs.strike;
    cell.underline = vc.attrs.underline;

    cell.fg = xrgb32{vc.fg.rgb.red, vc.fg.rgb.green, vc.fg.rgb.blue};
    cell.bg = xrgb32{vc.bg.rgb.red, vc.bg.rgb.green, vc.bg.rgb.blue};

    return cell;
}

}

////////////////////////////////////////////////////////////////////////////////
vte::vte(struct dim dim) :
    vterm_{vterm_new(dim.height, dim.width), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)},
    dim_{dim}
{
    info() << "Virtual terminal size: " << dim_;

    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .moverect    = dispatch::move_rect,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .resize      = dispatch::resize,
        .sb_pushline = nullptr,
        .sb_popline  = nullptr,
        .sb_clear    = nullptr,
    };

    vterm_set_utf8(&*vterm_, true);

    vterm_screen_set_callbacks(screen_, &callbacks, this);
    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_ROW);

    vterm_screen_enable_reflow(screen_, true);
    vterm_screen_enable_altscreen(screen_, true);

    vterm_screen_reset(screen_, true);
}

vte::~vte() { }

void vte::write(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }
void vte::commit() { vterm_screen_flush_damage(screen_); }

void vte::resize(struct dim dim)
{
    dim_ = dim;
    info() << "Resizing vte to: " << dim_;
    vterm_set_size(&*vterm_, dim_.height, dim_.width);
}

void vte::change(int row)
{
    std::vector<cell> cells;
    cells.reserve(dim_.width);

    for (VTermPos pos{row, 0}; pos.col < dim_.width; ++pos.col)
    {
        VTermScreenCell vc;
        if (vterm_screen_get_cell(screen_, pos, &vc))
        {
            vterm_state_convert_color_to_rgb(state_, &vc.fg);
            vterm_state_convert_color_to_rgb(state_, &vc.bg);
            cells.push_back(to_cell(vc));
        }
    }

    row_cb_(row, cells);
}
