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

    fb(const asio::any_io_executor& ex, fb::num);

    constexpr auto dpi() const noexcept { return info_.dpi; }

    void present() { info_.update(); }

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

        void update();
    };

    struct scoped_mmap_ptr
    {
        color* data;
        std::size_t size;

        scoped_mmap_ptr(asio::posix::stream_descriptor&, std::size_t);
        ~scoped_mmap_ptr();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;
    scoped_screen_info info_;
    scoped_mmap_ptr fb_;
};
