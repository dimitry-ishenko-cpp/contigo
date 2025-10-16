////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "vte.hpp"

#include <cstring> // std::memcpy
#include <vterm.h>

// VTermScreenCell is declared as an anonymous struct by libvterm, so we have to
// resort to the below if we want to keep EPA happy
namespace detail { struct VTermScreenCell : ::VTermScreenCell { }; }

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
        for (auto row = rect.start_row; row < rect.end_row; ++row) vt->change_row(row, cols);
    }
    return true;
}

static int move_rect(VTermRect dst, VTermRect src, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->move_cb_) vt->move_cb_(src.start_row, src.end_row - src.start_row, dst.start_row - src.start_row);
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

static int scroll_push_line(int cols, const VTermScreenCell* cells, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->scroll_.size() >= vt->scroll_size_) vt->scroll_.pop_front();

    std::size_t size = cols;
    vt->scroll_.emplace_back(std::make_unique<detail::VTermScreenCell[]>(size));
    std::memcpy(vt->scroll_.back().get(), cells, size * sizeof(VTermScreenCell));

    return true;
}

static int scroll_pop_line(int cols, VTermScreenCell* cells, void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    if (vt->scroll_.empty()) return 0;

    std::size_t size = cols;
    std::memcpy(cells, vt->scroll_.back().get(), size * sizeof(VTermScreenCell));

    vt->scroll_.pop_back();
    return true;
}

static int scroll_clear(void* ctx)
{
    auto vt = static_cast<vte*>(ctx);
    vt->scroll_.clear();
    return true;
}

};

////////////////////////////////////////////////////////////////////////////////
vte::vte(const size& size) :
    vterm_{vterm_new(size.h, size.w), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)}
{
    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .moverect    = dispatch::move_rect,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .sb_pushline = dispatch::scroll_push_line,
        .sb_popline  = dispatch::scroll_pop_line,
        .sb_clear    = dispatch::scroll_clear,
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

void vte::scroll_size(std::size_t max) { while (scroll_.size() > max) scroll_.pop_front(); }

void vte::resize(const size& size)
{
    vterm_set_size(&*vterm_, size.h, size.w);
}

void vte::change_row(int row, unsigned cols)
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

            std::memcpy(cell->ch, vc.chars, sizeof(vc.chars));
            cell->width = vc.width;

            cell->bold  = vc.attrs.bold;
            cell->italic= vc.attrs.italic;
            cell->strike= vc.attrs.strike;
            cell->underline = static_cast<under>(vc.attrs.underline);

            cell->fg = color{vc.fg.rgb.red, vc.fg.rgb.green, vc.fg.rgb.blue};
            cell->bg = color{vc.bg.rgb.red, vc.bg.rgb.green, vc.bg.rgb.blue};
        }
    }

    row_cb_(row, cells);
}
