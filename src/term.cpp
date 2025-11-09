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
    cursor_[keyboard].state.visible = true;
    cursor_[keyboard].state.shape = vte::cursor::block;

    pty_ = std::make_unique<pty::device>(ex, size_.rows, size_.cols, std::move(options.login), std::move(options.args));

    tty_->on_acquired([&]{ enable(); });
    tty_->on_released([&]{ disable(); });
    tty_->on_data_received([&](auto data){ vte_->send(data); });

    drm_->on_vblank([&](){ commit(); });
    if (options.tty_activate) drm_->activate(*fb_);

    vte_->on_send_data([&](auto data){ pty_->send(data); });
    vte_->on_row_changed([&](auto row, auto col, auto cols){ update(row, col, cols); });
    vte_->on_cursor_moved([&](auto row, auto col){ move_cursor(keyboard, row, col); });
    vte_->on_cursor_changed([&](auto&& cursor){ change(keyboard, cursor); });
    vte_->on_size_changed([&](auto rows, auto cols){ pty_->resize(rows, cols); });

    pty_->on_data_received([&](auto data){ vte_->recv(data); });

    try // mouse is optional
    {
        mouse_ = std::make_unique<mouse::device>(ex, size_.rows, size_.cols);
        cursor_[mouse].state.visible = true;
        cursor_[mouse].state.shape = vte::cursor::block;

        mouse_->on_moved([&](auto row, auto col)
        {
            move_cursor(mouse, row, col);
            vte_->move_mouse(row, col);
        });
        mouse_->on_button_changed([&](auto button, auto state){ vte_->change(button, state); });

        vte_->on_size_changed([&](auto rows, auto cols){ mouse_->resize(rows, cols); });
    }
    catch (const std::exception& e) { err() << e.what(); }
}

void term::enable()
{
    info() << "Enabling screen rendering";
    enabled_ = true;

    drm_->enable();
    drm_->activate(*fb_);
}

void term::disable()
{
    info() << "Disabling screen rendering";
    enabled_ = false;

    drm_->disable();
}

void term::update(int row, int col, unsigned count)
{
    if (row >= 0 && row < size_.rows)
    {
        auto col_end = col + count;
        if (col < 0) col = 0;
        if (col_end > size_.cols) col_end = size_.cols;
        count = col_end - col;

        // grab extra cell on the left and on the right to allow for overhang
        if (col > 0) --col, ++count;
        if (col_end < size_.cols) ++col_end, ++count;

        int x = col * box_.width, y = row * box_.height;

        auto cells = vte_->cells(row, col, count);
        auto image = pango_->render(cells);
        fb_->image().fill(x, y, image);

        for (auto k : {keyboard, mouse})
            if (cursor_[k].row == row && cursor_[k].col >= col && cursor_[k].col < col_end)
                draw_cursor(k);

        // TODO track damage
    }
}

void term::commit()
{
    vte_->commit();
    if (enabled_) fb_->commit();
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
        if (!cells[n].chars[0] && cells[n - 1].width == 2) --n, --cursor_[k].col;

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
