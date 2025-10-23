////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "geom.hpp"

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>

////////////////////////////////////////////////////////////////////////////////
class drm
{
public:
    ////////////////////
    static constexpr auto name = "card";
    static constexpr auto path = "/dev/dri/card";
    using num = unsigned;

    ////////////////////
    drm(const asio::any_io_executor&, num);

    constexpr auto res() const noexcept { return dim{1920, 1080}; }
    constexpr auto dpi() const noexcept { return 96; }

    void disable();
    void enable();

    ////////////////////
    static num any() { return 0; }

private:
    ////////////////////
    asio::posix::stream_descriptor fd_;
};
