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
    tty_ = std::make_unique<tty::device>(ex, options.tty_num, options.tty_activate);
    drm_ = std::make_unique<drm::device>(ex, options.drm_num);

    width_ = drm_->mode().width;
    height_ = drm_->mode().height;
    fb_ = std::make_unique<drm::framebuf>(*drm_, width_, height_);

    pango_ = std::make_unique<pango::engine>(options.font, options.dpi.value_or(drm_->mode().dpi));

    cell_width_ = pango_->cell_width();
    cell_height_ = pango_->cell_height();
    auto vte_width = width_ / cell_width_, vte_height = height_ / cell_height_;
    vte_ = std::make_unique<vte::machine>(vte_width, vte_height);
    pty_ = std::make_unique<pty::device>(ex, vte_width, vte_height, std::move(options.login), std::move(options.args));

    tty_->on_acquire([&]{ enable(); });
    tty_->on_release([&]{ disable(); });
    tty_->on_read_data([&](auto data){ pty_->write(data); });

    drm_->on_vblank([&](){ commit(); });
    drm_->activate(*fb_);

    vte_->on_row_changed([&](auto row, auto cells){ change(row, cells); });
    vte_->on_size_changed([&](auto w, auto h){ pty_->resize(w, h); });

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

void term::change(int row, std::span<const vte::cell> cells)
{
    auto image = pango_->render_line(width_, cells);
    fb_->image().fill(0, row * cell_height_, image);

    // TODO track damage
}

void term::commit()
{
    vte_->commit();
    if (enabled_) fb_->commit();
}
