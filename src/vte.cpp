////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "vte.hpp"

////////////////////////////////////////////////////////////////////////////////
namespace vte
{

struct machine::dispatch
{

static int damage(VTermRect rect, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->row_cb_)
        for (auto row = rect.start_row; row < rect.end_row; ++row)
            vt->change(row);
    return true;
}

static int move_rect(VTermRect dst, VTermRect src, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
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
    auto vt = static_cast<machine*>(ctx);
    // TODO
    return true;
}

static int set_prop(VTermProp prop, VTermValue* val, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    // TODO
    return true;
}

static int bell(void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    // TODO
    return true;
}

static int resize(int rows, int cols, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->size_cb_) vt->size_cb_(cols, rows);
    return true;
}

};

////////////////////////////////////////////////////////////////////////////////
machine::machine(unsigned w, unsigned h) :
    vterm_{vterm_new(h, w)},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)},
    width_{w}, height_{h}
{
    info() << "Virtual terminal size: " << width_ << "x" << height_;

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

void machine::write(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }
void machine::commit() { vterm_screen_flush_damage(screen_); }

void machine::resize(unsigned w, unsigned h)
{
    width_ = w; height_ = h;
    info() << "Resizing vte to: " << width_ << "x" << height_;
    vterm_set_size(&*vterm_, height_, width_);
}

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

auto vtc_to_color(VTermState* state, VTermColor* vc)
{
    vterm_state_convert_color_to_rgb(state, vc);
    return color(vc->rgb.red << 8, vc->rgb.green << 8, vc->rgb.blue << 8, 0xffff);
}

}

////////////////////////////////////////////////////////////////////////////////
void machine::change(int row)
{
    std::vector<cell> cells;
    cells.reserve(width_);

    for (VTermPos pos{row, 0}; pos.col < width_; ++pos.col)
    {
        VTermScreenCell vc;
        if (vterm_screen_get_cell(screen_, pos, &vc))
        {
            cell cell;

            ucs4_to_utf8(cell.chars, vc.chars);
            cell.width = vc.width;

            cell.bold  = vc.attrs.bold;
            cell.italic= vc.attrs.italic;
            cell.strike= vc.attrs.strike;
            cell.underline = vc.attrs.underline;

            cell.fg = vtc_to_color(state_, &vc.fg);
            cell.bg = vtc_to_color(state_, &vc.bg);

            cells.push_back(std::move(cell));
        }
    }

    row_cb_(row, cells);
}

////////////////////////////////////////////////////////////////////////////////
}
