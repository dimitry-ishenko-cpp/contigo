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
#include <string>
#include <vector>

#include <sys/types.h> // pid_t

////////////////////////////////////////////////////////////////////////////////
namespace pty
{

class device
{
public:
    ////////////////////
    // NB: passing by value here to have own copy with guaranteed lifetime
    device(const asio::any_io_executor&, unsigned rows, unsigned cols, std::string pgm, std::vector<std::string> args);
    ~device() { stop_child(); }

    using data_received_callback = std::function<void(std::span<const char>)>;
    void on_data_received(data_received_callback cb) { recv_cb_ = std::move(cb); }

    using child_exited_callback = std::function<void(int exit_code)>;
    void on_child_exited(child_exited_callback cb) { child_cb_ = std::move(cb); }

    void send(std::span<const char>);

    void resize(unsigned rows, unsigned cols);

private:
    ////////////////////
    asio::posix::stream_descriptor fd_;

    std::array<char, 4096> buffer_;
    data_received_callback recv_cb_;

    void sched_async_read();

    pid_t child_pid_;
    asio::posix::stream_descriptor child_fd_;
    child_exited_callback child_cb_;

    void start_child(std::string pgm, std::vector<std::string> args);
    void stop_child();
    void sched_child_wait();
};

}
