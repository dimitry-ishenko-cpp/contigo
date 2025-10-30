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
        for (auto y = rect.start_row; y < rect.end_row; ++y)
            vt->row_cb_(y);
    return true;
}

static int move_cursor(VTermPos pos, VTermPos old_pos, int visible, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);

    vt->cursor_.x = pos.col;
    vt->cursor_.y = pos.row;
    vt->cursor_.visible = visible;
    if (vt->cursor_cb_) vt->cursor_cb_(vt->cursor_);

    return true;
}

static int set_prop(VTermProp prop, VTermValue* val, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    bool cb = !!vt->cursor_cb_;

    switch (prop)
    {
        case VTERM_PROP_CURSORBLINK: vt->cursor_.blink = val->boolean; break;
        case VTERM_PROP_CURSORSHAPE:
            switch (val->number)
            {
                case VTERM_PROP_CURSORSHAPE_BAR_LEFT: vt->cursor_.shape = cursor::vline; break;
                case VTERM_PROP_CURSORSHAPE_BLOCK: vt->cursor_.shape = cursor::block; break;
                case VTERM_PROP_CURSORSHAPE_UNDERLINE: vt->cursor_.shape = cursor::hline; break;
                default: cb = false;
            }
            break;
        case VTERM_PROP_CURSORVISIBLE: vt->cursor_.visible = val->boolean; break;
        default: cb = false;
    }
    if (cb) vt->cursor_cb_(vt->cursor_);

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
    vterm_{vterm_new(h, w), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)},
    width_{w}
{
    info() << "Virtual terminal size: " << w << "x" << h;

    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .resize      = dispatch::resize,
    };

    vterm_set_utf8(&*vterm_, true);

    vterm_screen_set_callbacks(screen_, &callbacks, this);
    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_SCROLL);

    vterm_screen_enable_reflow(screen_, true);
    vterm_screen_enable_altscreen(screen_, true);

    vterm_screen_reset(screen_, true);
}

void machine::write(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }
void machine::commit() { vterm_screen_flush_damage(screen_); }

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

vte::cell machine::cell(int x, int y)
{
    vte::cell cell;

    VTermScreenCell vtc;
    if (vterm_screen_get_cell(screen_, VTermPos{y, x}, &vtc))
    {
        ucs4_to_utf8(cell.chars, vtc.chars);
        cell.width = vtc.width;

        cell.bold  = vtc.attrs.bold;
        cell.italic= vtc.attrs.italic;
        cell.strike= vtc.attrs.strike;
        cell.underline = vtc.attrs.underline;

        cell.fg = vtc_to_color(state_, &vtc.fg);
        cell.bg = vtc_to_color(state_, &vtc.bg);
    }

    return cell;
}

std::vector<vte::cell> machine::row(int y)
{
    std::vector<vte::cell> cells;
    cells.reserve(width_);

    for (auto x = 0; x < width_; ++x) cells.push_back(cell(x, y));

    return cells;
}

void machine::resize(unsigned w, unsigned h)
{
    info() << "Resizing vte to: " << w << "x" << h;
    vterm_set_size(&*vterm_, h, width_ = w);
}

////////////////////////////////////////////////////////////////////////////////
}
