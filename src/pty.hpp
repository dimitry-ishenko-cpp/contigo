////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <functional>
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
    ~pty() { if (pid_) stop_child(); }

    using child_exit_callback = std::function<void(int exit_code)>;
    void on_child_exit(child_exit_callback cb) { cb_ = std::move(cb); }

private:
    ////////////////////
    asio::posix::stream_descriptor pt_;

    ////////////////////
    pid_t pid_;
    asio::posix::stream_descriptor fd_;
    child_exit_callback cb_;

    void start_child(std::string pgm, std::vector<std::string> args);
    void stop_child();
    void handle_child_exit();
};
