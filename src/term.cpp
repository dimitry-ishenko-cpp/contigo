////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "term.hpp"

////////////////////////////////////////////////////////////////////////////////
term::term(const asio::any_io_executor& ex, term_options options) :
    tty_{std::make_shared<tty::device>(ex, options.tty_num)},
    tty_active_{tty_, options.tty_num, options.tty_activate},
    tty_raw_{tty_},
    tty_switch_{tty_},
    tty_graph_{tty_},

    drm_{std::make_shared<drm::device>(ex, options.drm_num)},
    drm_crtc_{drm_},
    drm_fb_{drm_},

    engine_{options.font, drm_->mode().dim.width, options.dpi.value_or(drm_->mode().dpi)},
    vte_{drm_->mode().dim / engine_.dim_cell()},
    pty_{ex, vte_.dim(), std::move(options.login), std::move(options.args)}
{
    tty_->on_read_data([&](std::span<const char> data){ pty_.write(data); });

    tty_switch_.on_acquire([&]{ enable(); });
    tty_switch_.on_release([&]{ disable(); });

    drm_crtc_.activate(drm_fb_);

    vte_.on_row_changed([&](int row, std::span<const cell> cells){ change(row, cells); });
    vte_.on_rows_moved([&](int row, unsigned rows, int distance){ move(row, rows, distance); });
    vte_.on_size_changed([&](dim dim){ pty_.resize(dim); });

    pty_.on_read_data([&](std::span<const char> data){ vte_.write(data); });
}

void term::enable()
{
    info() << "Enabling screen rendering";
    enabled_ = true;

    drm_->enable();
    drm_crtc_.activate(drm_fb_);
}

void term::disable()
{
    info() << "Disabling screen rendering";
    enabled_ = false;

    drm_->disable();
}

void term::change(int row, std::span<const cell> cells)
{
    // TODO
}

void term::move(int row, unsigned rows, int distance)
{
    // TODO
}
