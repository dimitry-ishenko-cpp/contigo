////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "term.hpp"

term::term(const asio::any_io_executor& ex, tty::num num, term_options options)
{
    tty_ = options.activate ? std::make_unique<tty>(ex, num, tty::activate) : std::make_unique<tty>(ex, num);
    pty_ = std::make_unique<pty>(ex, std::move(options.login), std::move(options.args));
    vte_ = std::make_unique<vte>(size{80, 24});

    tty_->on_read_data([&](auto data){ pty_->write(data); });
    pty_->on_read_data([&](auto data){ vte_->write(data); });
}
