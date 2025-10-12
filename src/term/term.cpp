////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "term.hpp"

term::term(const asio::any_io_executor& ex, tty::num num, const term_options options) :
    tty_{ options.activate ? tty{ex, num, tty::activate} : tty{ex, num} },
    pty_{ ex, std::move(options.login), std::move(options.args) },
    vte_{ 24, 80 }
{
    tty_.on_read_data([&](auto data){ pty_.write(data); });
    pty_.on_read_data([&](auto data){ vte_.write(data); });
}

term::term(const asio::any_io_executor& ex, term_options options) :
    term{ex, tty::active(ex), std::move(options)}
{ }
