////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <termios.h>

////////////////////////////////////////////////////////////////////////////////
using tty_num = unsigned;

class tty
{
public:
    ////////////////////
    enum action { dont_activate, activate };

    tty(const asio::any_io_executor&, tty_num, action = dont_activate);

    ////////////////////
    constexpr auto num() const noexcept { return num_; }

    static tty_num active(const asio::any_io_executor&);

private:
    ////////////////////
    struct scoped_active
    {
        asio::posix::stream_descriptor& vt;
        tty_num old_num;
        bool active = false;

        scoped_active(asio::posix::stream_descriptor&, tty_num num, tty::action);
        ~scoped_active();

        void activate(tty_num);
    };

    struct scoped_raw_state
    {
        asio::posix::stream_descriptor& vt;
        termios old_state;

        scoped_raw_state(asio::posix::stream_descriptor&);
        ~scoped_raw_state();
    };

    struct scoped_graph_mode
    {
        asio::posix::stream_descriptor& vt;
        unsigned old_mode;

        scoped_graph_mode(asio::posix::stream_descriptor&);
        ~scoped_graph_mode();
    };

    ////////////////////
    tty_num num_;
    asio::posix::stream_descriptor vt_;

    scoped_active active_;
    scoped_raw_state state_;
    scoped_graph_mode mode_;
};
