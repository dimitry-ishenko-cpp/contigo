////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "term.hpp"

term::term(const asio::any_io_executor& ex, term_options options)
{
    tty_ = std::make_unique<tty>(ex, options.tty_num, options.tty_activate);
    tty_->on_acquire([&]{ enable(); });
    tty_->on_release([&]{ disable(); });

    // TODO create drm, and get real res and dpi
    dim res{1920, 1080};
    unsigned dpi = 96;

    pango_ = std::make_unique<pango>(options.font, res, options.dpi.value_or(dpi));

    auto dim_cell = pango_->dim_cell();
    auto dim_vte = dim{res.width / dim_cell.width, res.height / dim_cell.height};

    vte_ = std::make_unique<vte>(dim_vte);
    vte_->on_row_changed([&](int row, std::span<const cell> cells){ change(row, cells); });
    vte_->on_rows_moved([&](int row, unsigned rows, int distance){ move(row, rows, distance); });
    vte_->redraw();

    pty_ = std::make_unique<pty>(ex, std::move(options.login), std::move(options.args));

    tty_->on_read_data([&](auto data){ pty_->write(data); });
    pty_->on_read_data([&](auto data){ vte_->write(data); });
}

void term::enable()
{
    info() << "Enabling screen rendering";
    enabled_ = true;

    vte_->redraw();
}

void term::disable()
{
    info() << "Disabling screen rendering";
    enabled_ = false;
}

void term::change(int row, std::span<const cell> cells)
{
    // TODO
}

void term::move(int row, unsigned rows, int distance)
{
    // TODO
}
