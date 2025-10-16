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
#include <asio/signal_set.hpp>
#include <functional>
#include <span>

#include <termios.h>

////////////////////////////////////////////////////////////////////////////////
class tty
{
public:
    ////////////////////
    static constexpr auto name = "tty";
    static constexpr auto path = "/dev/tty";
    using num = unsigned;

    struct activate_t { constexpr activate_t() = default; };
    static constexpr activate_t activate{};

    ////////////////////
    tty(const asio::any_io_executor& ex, tty::num num) : tty{ex, num, false} { }
    tty(const asio::any_io_executor& ex, tty::num num, activate_t) : tty{ex, num, true} { }

    using release_callback = std::function<void()>;
    void on_release(release_callback cb) { release_cb_ = std::move(cb); }

    using acquire_callback = std::function<void()>;
    void on_acquire(acquire_callback cb) { acquire_cb_ = std::move(cb); }

    using read_data_callback = std::function<void(std::span<const char>)>;
    void on_read_data(read_data_callback cb) { read_cb_ = std::move(cb); }

    ////////////////////
    static num active(const asio::any_io_executor&);

private:
    ////////////////////
    tty(const asio::any_io_executor&, tty::num, bool activate);

    ////////////////////
    struct scoped_active_vt // VT_ACTIVATE
    {
        asio::posix::stream_descriptor& fd;
        tty::num old_num, num;
        bool active = false;

        scoped_active_vt(asio::posix::stream_descriptor&, tty::num, bool activate);
        ~scoped_active_vt();

        void make_active(tty::num);
    };

    struct scoped_attrs // termios
    {
        asio::posix::stream_descriptor& fd;
        termios old_attrs;

        scoped_attrs(asio::posix::stream_descriptor&);
        ~scoped_attrs();
    };

    struct scoped_vt_mode // VT_SETMODE
    {
        asio::posix::stream_descriptor& fd;

        scoped_vt_mode(asio::posix::stream_descriptor&);
        ~scoped_vt_mode();
    };

    struct scoped_kd_mode // KDSETMODE
    {
        asio::posix::stream_descriptor& fd;
        unsigned old_mode;

        scoped_kd_mode(asio::posix::stream_descriptor&);
        ~scoped_kd_mode();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;

    scoped_active_vt active_;
    scoped_attrs attrs_;
    scoped_vt_mode vt_mode_;
    scoped_kd_mode kd_mode_;

    asio::signal_set sigs_;
    void sched_signal_callback();

    release_callback release_cb_;
    acquire_callback acquire_cb_;

    ////////////////////
    read_data_callback read_cb_;
    std::array<char, 4096> buffer_;

    void sched_async_read();
};
