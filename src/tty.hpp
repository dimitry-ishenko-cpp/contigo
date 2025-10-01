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
public:
    ////////////////////
    explicit tty(const asio::any_io_executor& ex) : tty{ex, get_active(ex)} { }
    tty(const asio::any_io_executor&, unsigned num);

    ////////////////////
    constexpr auto num() const noexcept { return num_; }
    void activate();

private:
    ////////////////////
    asio::posix::stream_descriptor vt_;
    unsigned num_;

    static unsigned get_active(const asio::any_io_executor&);
};

////////////////////////////////////////////////////////////////////////////////
#endif
