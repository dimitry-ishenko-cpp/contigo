////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "term.hpp"

#include <exception>

////////////////////////////////////////////////////////////////////////////////
term::term(const asio::any_io_executor& ex, term_options options)
{
    tty_ = std::make_unique<tty::device>(ex, options.tty_num);
    if (options.tty_activate) tty_->activate();

    drm_ = std::make_unique<drm::device>(ex, options.drm_num);
    mode_ = drm_->mode();
    fb_ = std::make_unique<drm::framebuf>(*drm_, mode_.width, mode_.height);

    pango_ = std::make_unique<pango::engine>(options.font, options.dpi.value_or(mode_.dpi));
    box_ = pango_->box();

    size_.rows = mode_.height / box_.height;
    size_.cols = mode_.width / box_.width;

    vte_ = std::make_unique<vte::machine>(size_.rows, size_.cols);
    show_cursor(keyboard);

    pty_ = std::make_unique<pty::device>(ex, size_.rows, size_.cols, std::move(options.login), std::move(options.args));

    tty_->on_acquired([&]{ activate(); });
    tty_->on_released([&]{ deactivate(); });
    tty_->on_data_received([&](auto data){ vte_->send(data); });

    drm_->on_vblank([&](){ update(); });
    if (options.tty_num == tty::active(ex)) activate();

    vte_->on_send_data([&](auto data){ pty_->send(data); });
    vte_->on_row_changed([&](auto row, auto col, auto cols){ update(row, col, cols); });
    vte_->on_cursor_moved([&](auto row, auto col)
    {
        hide_cursor(mouse);
        move_cursor(keyboard, row, col);
    });
    vte_->on_cursor_changed([&](auto&& cursor){ change(keyboard, cursor); });
    vte_->on_size_changed([&](auto rows, auto cols){ pty_->resize(rows, cols); });

    pty_->on_data_received([&](auto data){ vte_->recv(data); });

    try // mouse is optional
    {
        mouse_ = std::make_unique<mouse::device>(ex, size_.rows, size_.cols, options.mouse_speed);

        mouse_->on_moved([&](auto row, auto col)
        {
            show_cursor(mouse);
            move_cursor(mouse, row, col);
            vte_->move_mouse(row, col);
        });
        mouse_->on_button_changed([&](auto button, auto state){ vte_->change(button, state); });

        vte_->on_size_changed([&](auto rows, auto cols){ mouse_->resize(rows, cols); });
    }
    catch (const std::exception& e) { err() << e.what(); }
}

void term::activate()
{
    info() << "Activating terminal";
    active_ = true;

    drm_->acquire_master();
    drm_->set_output(*fb_);
}

void term::deactivate()
{
    info() << "Deactivating terminal";
    active_ = false;

    drm_->drop_master();
}

namespace
{

inline bool is_blank(const vte::cell& cell)
{
    return !cell.len || cell.chars[0] == ' ' || cell.attrs.conceal;
}

}

void term::update(int row, int col, unsigned count)
{
    if (row >= 0 && row < size_.rows)
    {
        auto col_end = col + count;
        if (col < 0) col = 0;
        if (col_end > size_.cols) col_end = size_.cols;

        // grab extra cell before and after to deal with overhang
        bool before = (col > 0); if (before) --col;
        bool after = (col_end < size_.cols); if (after) ++col_end;

        auto cells = vte_->cells(row, col, col_end - col);

        if (before && is_blank(*cells.begin()))
        {
            cells.erase(cells.begin());
            ++col;
        }
        if (after && !is_blank(*cells.rbegin()))
        {
            cells.pop_back();
            --col_end;
        }

        auto image = pango_->render(cells);

        int x = col * box_.width, y = row * box_.height;
        fb_->image().fill(x, y, image);

        for (auto k : {keyboard, mouse})
            if (cursor_[k].row == row && cursor_[k].col >= col && cursor_[k].col < col_end)
                draw_cursor(k);

        // TODO track damage
    }
}

void term::update()
{
    vte_->commit();
    if (active_) fb_->commit();
}

void term::move_cursor(kind k, int row, int col)
{
    undraw_cursor(k);
    cursor_[k].row = row;
    cursor_[k].col = col;
    draw_cursor(k);
}

void term::change(kind k, const vte::cursor& state)
{
    undraw_cursor(k);
    cursor_[k].state = state;
    draw_cursor(k);
}

void term::show_cursor(kind k) { cursor_[k].state.visible = true; }
void term::hide_cursor(kind k) { cursor_[k].state.visible = false; undraw_cursor(k); }

void term::draw_cursor(kind k)
{
    auto& cursor = cursor_[k];
    auto& patch = patch_[k];

    if (cursor.state.visible)
    {
        // the cursor can land on one of the following:
        //   1. normal cell => render this cell
        //   2. wide   cell => render this cell and the next one (empty)
        //   3. empty  cell => if the prior cell is wide, render that cell and this one
        //
        auto cells = vte_->cells(cursor.row, cursor.col - 1, 3);
        auto n = 1;
        if (!cells[n].len && cells[n - 1].width == 2) --n, --cursor_[k].col;

        auto& cell = cells[n];

        auto x = cursor.col * box_.width, y = cursor.row * box_.height;
        auto w = box_.width * cell.width, h = box_.height;

        patch = pixman::image{w, h};
        patch->fill(0, 0, fb_->image(), x, y, w, h);

        switch (cursor.state.shape)
        {
        case vte::cursor::block:
            std::swap(cell.fg, cell.bg);
            fb_->image().fill(x, y, pango_->render(std::span{&cell, cell.width}));
            break;

        case vte::cursor::vline:
            fb_->image().fill(x, y, 2, h, cell.fg);
            break;

        case vte::cursor::hline:
            fb_->image().fill(x, y + h - 2, w, 2, cell.fg);
            break;
        };
    }
}

void term::undraw_cursor(kind k)
{
    if (patch_[k])
    {
        auto x = cursor_[k].col * box_.width, y = cursor_[k].row * box_.height;
        fb_->image().fill(x, y, *patch_[k]);
        patch_[k].reset();
    }
}
