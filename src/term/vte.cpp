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
        VTermPos pos;
        for (pos.row = rect.start_row; pos.row < rect.end_row; ++pos.row)
        {
            std::vector<cell> cells;
            for (pos.col = rect.start_col; pos.col < rect.end_col; ++pos.col)
            {
                VTermScreenCell vc;
                if (vterm_screen_get_cell(vt->screen_, pos, &vc))
                {
                    vterm_state_convert_color_to_rgb(vt->state_, &vc.fg);
                    vterm_state_convert_color_to_rgb(vt->state_, &vc.bg);

                    // TODO: fill cells
                }
            }
            vt->row_cb_(pos.row, cells);
        }
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

static int resize(int rows, int cols, void* ctx)
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
vte::vte(size size) :
    vterm_{vterm_new(size.h, size.w), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)}
{
    vterm_set_utf8(&*vterm_, true);

    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .moverect    = dispatch::move_rect,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .resize      = dispatch::resize,
        .sb_pushline = dispatch::scroll_push_line,
        .sb_popline  = dispatch::scroll_pop_line,
        .sb_clear    = dispatch::scroll_clear,
    };
    vterm_screen_set_callbacks(screen_, &callbacks, this);

    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_ROW);
    vterm_screen_reset(screen_, true);
}

vte::~vte() { }

void vte::write(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }
void vte::flush() { vterm_screen_flush_damage(screen_); }

void vte::scroll_size(std::size_t max) { while (scroll_.size() > max) scroll_.pop_front(); }
