////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <linux/fb.h>

////////////////////////////////////////////////////////////////////////////////
class fb
{
public:
    ////////////////////
    using num = unsigned;
    struct size { unsigned x, y; };

    fb(const asio::any_io_executor& ex, fb::num);

    constexpr auto res() const noexcept { return size{info_.vinfo.xres, info_.vinfo.yres}; }
    constexpr auto dpi() const noexcept { return info_.dpi; }

private:
    ////////////////////
    struct scoped_screen_info
    {
        asio::posix::stream_descriptor& fd;
        fb_fix_screeninfo finfo;
        fb_var_screeninfo old_vinfo, vinfo;

        unsigned dpi = 96;

        scoped_screen_info(asio::posix::stream_descriptor&);
        ~scoped_screen_info();
    };

    struct scoped_mmap_ptr
    {
        xrgb32* data;
        std::size_t size;
        std::size_t stride;

        scoped_mmap_ptr(asio::posix::stream_descriptor&, std::size_t size_bytes, std::size_t stride_bytes);
        ~scoped_mmap_ptr();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;
    scoped_screen_info info_;
    scoped_mmap_ptr fb_;
};
