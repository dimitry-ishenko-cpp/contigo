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

    vte_ = std::make_unique<vte>(dim{80, 24});
    vte_->on_row_changed([&](int row, std::span<const cell> cells){ draw_row(row, cells); });
    vte_->on_rows_moved([&](int row, unsigned rows, int distance){ move_rows(row, rows, distance); });
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

void term::draw_row(int row, std::span<const cell> cells)
{
}

void term::move_rows(int row, unsigned rows, int distance)
{
}
