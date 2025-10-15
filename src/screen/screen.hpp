////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <optional>

#include "fb.hpp"

////////////////////////////////////////////////////////////////////////////////
struct screen_options
{
    std::optional<unsigned> dpi;
};

class screen
{
public:
    ////////////////////
    screen(const asio::any_io_executor&, fb::num, screen_options = {});

    void enable(bool = true);
    void disable() { enable(false); }

    constexpr auto dpi() const noexcept { return dpi_; }

private:
    ////////////////////
    fb fb_;
    bool enabled_ = false;

    unsigned dpi_;
};
