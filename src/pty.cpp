////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "logging.hpp"
#include "pty.hpp"

#include <asio.hpp>
#include <thread>

#include <pty.h> // forkpty
#include <sys/syscall.h> // pidfd_open, syscall
#include <sys/wait.h> // waitpid
#include <unistd.h> // execv

using namespace std::chrono_literals;

////////////////////////////////////////////////////////////////////////////////
namespace 
{

inline int pidfd_open(pid_t pid, unsigned flags)
{
    return syscall(SYS_pidfd_open, pid, flags);
}

}

////////////////////////////////////////////////////////////////////////////////
pty::pty(const asio::any_io_executor& ex, std::string pgm, std::vector<std::string> args) :
    pt_{ex}, child_fd_{ex}
{
    info() << "Spawning child process";

    int pt;
    child_pid_ = forkpty(&pt, nullptr, nullptr, nullptr);
    if (child_pid_ < 0) throw posix_error{"forkpty"};

    if (child_pid_ > 0) // parent
    {
        pt_.assign(pt);
        sched_async_read();

        /////////////////////
        auto fd = pidfd_open(child_pid_, 0);
        if (fd < 0) throw posix_error{"pidfd_open"};

        child_fd_.assign(fd);
        child_fd_.async_wait(child_fd_.wait_read, [&](std::error_code ec){ if (!ec) handle_child_exit(); });
    }
    else start_child(std::move(pgm), std::move(args));
}

void pty::write(std::span<const char> data)
{
    asio::write(pt_, asio::buffer(data));
}

void pty::sched_async_read()
{
    pt_.async_read_some(asio::buffer(buffer_), [&](std::error_code ec, std::size_t size)
    {
        if (!ec)
        {
            if (read_cb_) read_cb_(std::span<const char>{buffer_.begin(), size});
            sched_async_read();
        }
    });
}

////////////////////////////////////////////////////////////////////////////////
void pty::start_child(std::string pgm, std::vector<std::string> args)
{
    std::vector<char*> argv = { pgm.data() };
    for (auto&& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);

    execv(argv[0], argv.data());
    throw posix_error{"execv"};
}

void pty::stop_child()
{
    if (child_pid_)
    {
        info() << "Terminating child process";
        kill(child_pid_, SIGTERM);

        for (auto i = 0; i < 10; ++i)
        {
            std::this_thread::sleep_for(10ms);

            auto pid = waitpid(child_pid_, nullptr, WNOHANG);
            if (pid == child_pid_ || (pid == -1 && errno == ECHILD))
            {
                child_pid_ = 0;
                break;
            }
        }

        if (child_pid_)
        {
            info() << "Killing child process";
            kill(child_pid_, SIGKILL);
        }
    }
}

void pty::handle_child_exit()
{
    int status;
    waitpid(child_pid_, &status, 0);
    child_pid_ = 0;
    
    int exit_code = 0;
    if (WIFEXITED(status))
        exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        exit_code = 128 + WTERMSIG(status);

    info() << "Child process exited with code " << exit_code;
    if (child_cb_) child_cb_(exit_code);
}
