////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "term.hpp"

////////////////////////////////////////////////////////////////////////////////
term::term(const asio::any_io_executor& ex, term_options options)
{
    tty_ = std::make_unique<tty::device>(ex, options.tty_num);

    drm_ = std::make_unique<drm::device>(ex, options.drm_num);
    mode_ = drm_->mode();
    fb_ = std::make_unique<drm::framebuf>(*drm_, mode_.width, mode_.height);

    pango_ = std::make_unique<pango::engine>(options.font, options.dpi.value_or(mode_.dpi));
    cell_ = pango_->cell();

    auto vte_rows = mode_.height / cell_.height;
    auto vte_cols = mode_.width / cell_.width;
    vte_ = std::make_unique<vte::machine>(vte_rows, vte_cols);
    pty_ = std::make_unique<pty::device>(ex, vte_rows, vte_cols, std::move(options.login), std::move(options.args));

    tty_->on_acquire([&]{ enable(); });
    tty_->on_release([&]{ disable(); });
    tty_->on_read_data([&](auto data){ pty_->write(data); });

    drm_->on_vblank([&](){ commit(); });
    if (options.tty_activate)
    {
        tty_->activate();
        drm_->activate(*fb_);
    }

    vte_->on_output_data([&](auto data){ pty_->write(data); });
    vte_->on_row_changed([&](auto row){ change(row); });
    vte_->on_cursor_changed([&](auto&& cursor){ undo_cursor(); cursor_ = cursor; draw_cursor(); });
    vte_->on_size_changed([&](auto rows, auto cols){ pty_->resize(rows, cols); });

    pty_->on_read_data([&](auto data){ vte_->write(data); });
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

void term::change(int row)
{
    auto image = pango_->render_line(mode_.width, vte_->cells(row));
    fb_->image().fill(0, row * cell_.height, image);

    if (row == cursor_.row) draw_cursor();

    // TODO track damage
}

void term::draw_cursor()
{
    if (cursor_.visible)
    {
        auto x = cursor_.col * cell_.width, y = cursor_.row * cell_.height; 

        undo_ = pixman::image{cell_.width, cell_.height};
        undo_->fill(0, 0, fb_->image(), x, y, cell_.width, cell_.height);

        auto cell = vte_->cell(cursor_.row, cursor_.col);
        switch (cursor_.shape)
        {
        case vte::cursor::block:
            {
                std::swap(cell.fg, cell.bg); // TODO use reverse attr
                auto image = pango_->render_line(cell_.width, std::span{&cell, 1});
                fb_->image().fill(x, y, image);
            }
            break;

        case vte::cursor::vline:
            fb_->image().fill(x, y, 2, cell_.height, cell.fg);
            break;

        case vte::cursor::hline:
            fb_->image().fill(x, y + cell_.height - 2, cell_.width, 2, cell.fg);
            break;
        };
    }
}

void term::undo_cursor()
{
    if (undo_)
    {
        fb_->image().fill(cursor_.col * cell_.width, cursor_.row * cell_.height, *undo_);
        undo_.reset();
    }
}

void term::commit()
{
    vte_->commit();
    if (enabled_) fb_->commit();
}
