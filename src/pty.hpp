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
class pty
{
public:
    ////////////////////
    // NB: passing by value here to have own copy with guaranteed lifetime
    pty(const asio::any_io_executor&, std::string pgm, std::vector<std::string> args);
    ~pty() { stop_child(); }

    using read_data_callback = std::function<void(std::span<const char>)>;
    void on_read_data(read_data_callback cb) { read_cb_ = std::move(cb); }

    using child_exit_callback = std::function<void(int exit_code)>;
    void on_child_exit(child_exit_callback cb) { child_cb_ = std::move(cb); }

    void write(std::span<const char>);

private:
    ////////////////////
    asio::posix::stream_descriptor fd_;
    read_data_callback read_cb_;
    std::array<char, 4096> buffer_;

    void sched_async_read();

    ////////////////////
    pid_t child_pid_;
    asio::posix::stream_descriptor child_fd_;
    child_exit_callback child_cb_;

    void start_child(std::string pgm, std::vector<std::string> args);
    void stop_child();
    void sched_child_wait();
};
