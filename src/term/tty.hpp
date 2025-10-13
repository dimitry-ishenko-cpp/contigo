////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <array>
#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <functional>
#include <span>

#include <termios.h>

////////////////////////////////////////////////////////////////////////////////
class tty
{
public:
    ////////////////////
    using num = unsigned;

    struct activate_t { constexpr activate_t() = default; };
    static constexpr activate_t activate{};

    tty(const asio::any_io_executor& ex, tty::num num) : tty{ex, num, false} { } 
    tty(const asio::any_io_executor& ex, tty::num num, activate_t) : tty{ex, num, true} { }

    using read_data_callback = std::function<void(std::span<const char>)>;
    void on_read_data(read_data_callback cb) { read_cb_ = std::move(cb); }

    ////////////////////
    static num active(const asio::any_io_executor&);

private:
    ////////////////////
    tty(const asio::any_io_executor&, tty::num, bool activate);

    ////////////////////
    struct scoped_active // VT_ACTIVATE
    {
        asio::posix::stream_descriptor& fd;
        tty::num old_num;
        bool active = false;

        scoped_active(asio::posix::stream_descriptor&, tty::num, bool activate);
        ~scoped_active();

        void make_active(tty::num);
    };

    struct scoped_raw_state // termios
    {
        asio::posix::stream_descriptor& fd;
        termios old_state;

        scoped_raw_state(asio::posix::stream_descriptor&);
        ~scoped_raw_state();
    };

    struct scoped_graphic_mode // KD_GRAPHICS
    {
        asio::posix::stream_descriptor& fd;
        unsigned old_mode;

        scoped_graphic_mode(asio::posix::stream_descriptor&);
        ~scoped_graphic_mode();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;

    scoped_active active_;
    scoped_raw_state raw_;
    scoped_graphic_mode graphic_;

    read_data_callback read_cb_;
    std::array<char, 4096> buffer_;

    void sched_async_read();
};
