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
#include <optional>
#include <span>

#include <termios.h>

////////////////////////////////////////////////////////////////////////////////
namespace tty
{

constexpr auto name = "tty";
constexpr auto path = "/dev/tty";
using num = unsigned;

num active(const asio::any_io_executor&);

////////////////////////////////////////////////////////////////////////////////
class device
{
public:
    ////////////////////
    device(const asio::any_io_executor&, num);

    using released_callback = std::function<void()>;
    void on_released(released_callback cb) { release_cb_ = std::move(cb); }

    using acquired_callback = std::function<void()>;
    void on_acquired(acquired_callback cb) { acquire_cb_ = std::move(cb); }

    using data_received_callback = std::function<void(std::span<const char>)>;
    void on_data_received(data_received_callback cb) { recv_cb_ = std::move(cb); }

    void activate() { active_.activate(); }

private:
    ////////////////////
    struct scoped_active
    {
        asio::posix::stream_descriptor& fd;
        std::optional<tty::num> prev;
        tty::num num;

        void activate();
        void activate(tty::num);

        scoped_active(asio::posix::stream_descriptor&, tty::num);
        ~scoped_active();
    };

    struct scoped_raw_mode
    {
        asio::posix::stream_descriptor& fd;
        termios prev;

        scoped_raw_mode(asio::posix::stream_descriptor&);
        ~scoped_raw_mode();
    };

    struct scoped_process_switch
    {
        asio::posix::stream_descriptor& fd;

        scoped_process_switch(asio::posix::stream_descriptor&);
        ~scoped_process_switch();
    };

    struct scoped_graphics_mode
    {
        asio::posix::stream_descriptor& fd;
        unsigned prev;

        scoped_graphics_mode(asio::posix::stream_descriptor&);
        ~scoped_graphics_mode();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;

    scoped_active active_;
    scoped_raw_mode raw_mode_;
    scoped_process_switch proc_switch_;
    scoped_graphics_mode graph_mode_;

    released_callback release_cb_;
    acquired_callback acquire_cb_;

    asio::signal_set sigs_;
    void sched_signal_callback();

    std::array<char, 4096> buffer_;
    data_received_callback recv_cb_;

    void sched_async_read();
};

////////////////////////////////////////////////////////////////////////////////
}
