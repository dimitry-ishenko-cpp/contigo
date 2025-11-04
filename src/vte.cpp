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
    {
        auto col = rect.start_col, cols = rect.end_col - rect.start_col;
        for (auto row = rect.start_row; row < rect.end_row; ++row) vt->row_cb_(row, col, cols);
    }
    return true;
}

static int move_cursor(VTermPos pos, VTermPos old_pos, int visible, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);

    vt->cursor_.row = pos.row;
    vt->cursor_.col = pos.col;
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
    case VTERM_PROP_CURSORBLINK:
        vt->cursor_.blink = val->boolean;
        break;
    case VTERM_PROP_CURSORSHAPE:
        switch (val->number)
        {
        case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
            vt->cursor_.shape = cursor::vline;
            break;
        case VTERM_PROP_CURSORSHAPE_BLOCK:
            vt->cursor_.shape = cursor::block;
            break;
        case VTERM_PROP_CURSORSHAPE_UNDERLINE:
            vt->cursor_.shape = cursor::hline;
            break;
        default: cb = false;
        }
        break;
    case VTERM_PROP_CURSORVISIBLE:
        vt->cursor_.visible = val->boolean;
        break;
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
    if (vt->size_cb_) vt->size_cb_(rows, cols);
    return true;
}

static void output(const char* s, std::size_t len, void *ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->output_cb_) vt->output_cb_(std::span{s, len});
}

};

////////////////////////////////////////////////////////////////////////////////
machine::machine(unsigned rows, unsigned cols) :
    vterm_{vterm_new(rows, cols), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)}
{
    info() << "Virtual terminal size: " << rows << "x" << cols;

    vterm_set_utf8(&*vterm_, true);
    vterm_output_set_callback(&*vterm_, &dispatch::output, this);

    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .resize      = dispatch::resize,
    };

    vterm_screen_set_callbacks(screen_, &callbacks, this);
    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_SCROLL);

    vterm_screen_enable_reflow(screen_, true);
    vterm_screen_enable_altscreen(screen_, true);

    vterm_screen_reset(screen_, true);
}

void machine::write(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }
void machine::commit() { vterm_screen_flush_damage(screen_); }

std::vector<vte::cell> machine::cells(int row, int col, unsigned count)
{
    std::vector<vte::cell> cells;
    cells.reserve(count);

    for (; count; ++col, --count) cells.push_back(cell(row, col));

    return cells;
}

void machine::resize(unsigned rows, unsigned cols)
{
    info() << "Resizing vte to: " << rows << "x" << cols;
    vterm_set_size(&*vterm_, rows, cols);
}

namespace
{

void ucs4_to_utf8(const uint32_t* in, char* out, std::size_t* len)
{
    auto begin = out;
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
    *len = out - begin;
}

auto to_color(VTermState* state, VTermColor vc)
{
    vterm_state_convert_color_to_rgb(state, &vc);
    return pixman::color(vc.rgb.red << 8, vc.rgb.green << 8, vc.rgb.blue << 8, 0xffff);
}

}

vte::cell machine::cell(int row, int col)
{
    vte::cell cell;

    VTermScreenCell vtc;
    if (vterm_screen_get_cell(screen_, VTermPos{row, col}, &vtc))
    {
        ucs4_to_utf8(vtc.chars, cell.chars, &cell.len);
        cell.width = vtc.width;
        cell.attrs = vtc.attrs;
        cell.fg = to_color(state_, vtc.fg);
        cell.bg = to_color(state_, vtc.bg);
    }

    return cell;
}

////////////////////////////////////////////////////////////////////////////////
}
