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
#include <memory>
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

    auto& executor() { return fd_.get_executor(); }
    template<typename Cmd> void io_control(Cmd& cmd) { fd_.io_control(cmd); }
    auto handle() { return fd_.native_handle(); }

    using read_data_callback = std::function<void(std::span<const char>)>;
    void on_read_data(read_data_callback cb) { read_cb_ = std::move(cb); }

private:
    ////////////////////
    asio::posix::stream_descriptor fd_;

    read_data_callback read_cb_;
    std::array<char, 4096> buffer_;

    void sched_async_read();
};

////////////////////////////////////////////////////////////////////////////////
class scoped_active
{
    std::shared_ptr<device> dev_;
    num prev_, num_;
    bool active_ = false;

    void make_active(num);

public:
    ////////////////////
    scoped_active(std::shared_ptr<device>, num, bool activate);
    ~scoped_active();
};

////////////////////////////////////////////////////////////////////////////////
class scoped_raw_mode
{
    std::shared_ptr<device> dev_;
    termios prev_;

public:
    ////////////////////
    explicit scoped_raw_mode(std::shared_ptr<device>);
    ~scoped_raw_mode();
};

////////////////////////////////////////////////////////////////////////////////
class scoped_process_switch
{
public:
    ////////////////////
    explicit scoped_process_switch(std::shared_ptr<device>);
    ~scoped_process_switch();

    using release_callback = std::function<void()>;
    void on_release(release_callback cb) { release_cb_ = std::move(cb); }

    using acquire_callback = std::function<void()>;
    void on_acquire(acquire_callback cb) { acquire_cb_ = std::move(cb); }

private:
    ////////////////////
    std::shared_ptr<device> dev_;

    asio::signal_set sigs_;
    void sched_signal_callback();

    release_callback release_cb_;
    acquire_callback acquire_cb_;
};

////////////////////////////////////////////////////////////////////////////////
class scoped_graphics_mode
{
    std::shared_ptr<device> dev_;
    unsigned prev_;

public:
    ////////////////////
    explicit scoped_graphics_mode(std::shared_ptr<device>);
    ~scoped_graphics_mode();
};

////////////////////////////////////////////////////////////////////////////////
}
