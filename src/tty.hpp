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
#include <optional>

////////////////////////////////////////////////////////////////////////////////
class tty
{
public:
    ////////////////////
    enum action { dont_activate, activate };

    explicit tty(const asio::any_io_executor&, std::optional<unsigned> = {}, action = dont_activate);

    ////////////////////
    constexpr auto num() const noexcept { return num_; }

private:
    ////////////////////
    unsigned num_;
    asio::posix::stream_descriptor vt_;
};

////////////////////////////////////////////////////////////////////////////////
#endif
