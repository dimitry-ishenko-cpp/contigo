////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#ifndef TTY_HPP
#define TTY_HPP

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>

////////////////////////////////////////////////////////////////////////////////
class tty
{
    ////////////////////
    struct activate_t { explicit activate_t() = default; };

public:
    ////////////////////
    static constexpr activate_t activate{};

    ////////////////////
    explicit tty(const asio::any_io_executor& ex);
    tty(const asio::any_io_executor&, unsigned num);
    tty(const asio::any_io_executor&, unsigned num, activate_t);

    ////////////////////
    constexpr auto num() const noexcept { return num_; }

private:
    ////////////////////
    asio::posix::stream_descriptor vt_;
    unsigned num_;
};

////////////////////////////////////////////////////////////////////////////////
#endif
