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
class tty
{
public:
    ////////////////////
    using num = unsigned;
    enum action { dont_activate, activate };

    tty(const asio::any_io_executor&, num, action = dont_activate);

    ////////////////////
    static num active(const asio::any_io_executor&);

private:
    ////////////////////
    struct scoped_active
    {
        asio::posix::stream_descriptor& fd;
        tty::num old_num;
        bool active = false;

        scoped_active(asio::posix::stream_descriptor&, tty::num, tty::action);
        ~scoped_active();

        void activate(tty::num);
    };

    struct scoped_raw_state
    {
        asio::posix::stream_descriptor& fd;
        termios old_state;

        scoped_raw_state(asio::posix::stream_descriptor&);
        ~scoped_raw_state();
    };

    struct scoped_graphic_mode
    {
        asio::posix::stream_descriptor& fd;
        unsigned old_mode;

        scoped_graphic_mode(asio::posix::stream_descriptor&);
        ~scoped_graphic_mode();
    };

    ////////////////////
    asio::posix::stream_descriptor tty_fd_;

    scoped_active active_;
    scoped_raw_state state_;
    scoped_graphic_mode mode_;
};
