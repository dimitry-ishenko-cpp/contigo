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
    if (vt->row_cb_)
    {
        int rows, cols;
        vterm_get_size(&*vt->vterm_, &rows, &cols);
        for (auto row = rect.start_row; row < rect.end_row; ++row) vt->change(row, cols);
    }
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
    // info() << "cursor: " << old_pos.col << "," << old_pos.row << " => " << pos.col << "," << pos.row;
    return true;
}

static int set_prop(VTermProp prop, VTermValue* val, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    // TODO
    
    static auto to_bool = [](VTermValue* val){ return val->boolean ? "on" : "off"; };
    static auto to_string = [](VTermValue* val){ return std::string{val->string.str, val->string.len}; };
    static auto to_cursor_shape = [](VTermValue* val){
        switch (val->number)
        {
            case VTERM_PROP_CURSORSHAPE_BLOCK: return "block";
            case VTERM_PROP_CURSORSHAPE_UNDERLINE: return "underline";
            case VTERM_PROP_CURSORSHAPE_BAR_LEFT: return "bar-left";
            default: return "???";
        }
    };
    static auto to_mouse = [](VTermValue* val){
        switch (val->number)
        {
            case VTERM_PROP_MOUSE_NONE: return "none";
            case VTERM_PROP_MOUSE_CLICK: return "click";
            case VTERM_PROP_MOUSE_DRAG: return "drag";
            case VTERM_PROP_MOUSE_MOVE: return "move";
            default: return "???";
        }
    };

    switch (prop)
    {
        case VTERM_PROP_CURSORVISIBLE: info() << "cursor visible = " << to_bool(val); break;
        case VTERM_PROP_CURSORBLINK  : info() << "cursor blink = " << to_bool(val); break;
        case VTERM_PROP_ALTSCREEN    : info() << "alt screen = " << to_bool(val); break;
        case VTERM_PROP_TITLE        : info() << "title = " << to_string(val); break;
        case VTERM_PROP_ICONNAME     : info() << "icon name = " << to_string(val); break;
        case VTERM_PROP_REVERSE      : info() << "reverse = " << to_bool(val); break;
        case VTERM_PROP_CURSORSHAPE  : info() << "cursor shape = " << to_cursor_shape(val); break;
        case VTERM_PROP_MOUSE        : info() << "mouse = " << to_mouse(val); break;
        case VTERM_PROP_FOCUSREPORT  : info() << "focus report = " << to_bool(val); break;
        default: break;
    }
    return true;
}

static int bell(void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    // TODO
    info() << "bell";
    return true;
}

static int resize(int rows, int cols, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->size_cb_) vt->size_cb_(dim(cols, rows));
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

}

////////////////////////////////////////////////////////////////////////////////
vte::vte(dim dim) :
    vterm_{vterm_new(dim.height, dim.width), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)}
{
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

void vte::redraw()
{
    int rows, cols;
    vterm_get_size(&*vterm_, &rows, &cols);
    vterm_set_size(&*vterm_, rows, cols);
}

void vte::change(int row, unsigned cols)
{
    std::vector<cell> cells{cols};

    auto cell = cells.begin();
    for (VTermPos pos{row, 0}; pos.col < cols; ++pos.col, ++cell)
    {
        VTermScreenCell vc;
        if (vterm_screen_get_cell(screen_, pos, &vc))
        {
            vterm_state_convert_color_to_rgb(state_, &vc.fg);
            vterm_state_convert_color_to_rgb(state_, &vc.bg);

            ucs4_to_utf8(cell->chars, vc.chars);
            cell->width = vc.width;

            cell->bold  = vc.attrs.bold;
            cell->italic= vc.attrs.italic;
            cell->strike= vc.attrs.strike;
            cell->under = static_cast<underline>(vc.attrs.underline);

            cell->fg = color{vc.fg.rgb.red, vc.fg.rgb.green, vc.fg.rgb.blue};
            cell->bg = color{vc.bg.rgb.red, vc.bg.rgb.green, vc.bg.rgb.blue};
        }
    }

    row_cb_(row, cells);
}
