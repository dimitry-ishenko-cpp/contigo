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
    tty_{ex, options.tty_num, options.tty_activate},

    drm_{ex, options.drm_num},
    fb_{drm_, drm_.mode().width, drm_.mode().height},

    pango_{options.font, options.dpi.value_or(drm_.mode().dpi)},
    vte_{drm_.mode().width / pango_.cell_width(), drm_.mode().height / pango_.cell_height()},
    pty_{ex, vte_.width(), vte_.height(), std::move(options.login), std::move(options.args)}
{
    tty_.on_acquire([&]{ enable(); });
    tty_.on_release([&]{ disable(); });
    tty_.on_read_data([&](std::span<const char> data){ pty_.write(data); });

    drm_.activate(fb_);

    vte_.on_row_changed([&](int row, std::span<const vte::cell> cells){ change(row, cells); });
    vte_.on_cells_moved([&](int col, int row, unsigned w, unsigned h, int src_col, int src_row){ move(col, row, w, h, src_col, src_row); });
    vte_.on_size_changed([&](unsigned w, unsigned h){ pty_.resize(w, h); });

    pty_.on_read_data([&](std::span<const char> data){ vte_.write(data); vte_.commit(); });
}

void term::enable()
{
    info() << "Enabling screen rendering";
    enabled_ = true;

    drm_.enable();
    drm_.activate(fb_);
}

void term::disable()
{
    info() << "Disabling screen rendering";
    enabled_ = false;

    drm_.disable();
}

void term::change(int row, std::span<const vte::cell> cells)
{
    auto image = pango_.render_line(drm_.mode().width, cells);

    auto ch = pango_.cell_height();
    fb_.image().fill(0, row * ch, image);

    if (enabled_) fb_.commit();
}

void term::move(int col, int row, unsigned w, unsigned h, int src_col, int src_row)
{
    // TODO
}
