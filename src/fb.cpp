////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "command.hpp"
#include "error.hpp"
#include "fb.hpp"
#include "logging.hpp"

#include <asio/post.hpp>
#include <string>
#include <sstream>
#include <stdexcept>

#include <fcntl.h> // open
#include <linux/fb.h>
#include <sys/mman.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

inline auto open_device(const asio::any_io_executor& ex, fb::num num)
{
    auto path = fb::path + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

}

////////////////////////////////////////////////////////////////////////////////
fb::fb(const asio::any_io_executor& ex, fb::num num) :
    fd_{open_device(ex, num)}, info_{fd_}, fb_{fd_, info_.finfo.smem_len}
{
    thread_ = std::jthread{[&](std::stop_token st)
    {
        while (!st.stop_requested())
        {
            command<FBIO_WAITFORVSYNC, int> wait;
            std::error_code ec;

            fd_.io_control(wait, ec);
            if (ec) break;

            if (sync_cb_) asio::post(fd_.get_executor(), sync_cb_);
        }
    }};
}

////////////////////////////////////////////////////////////////////////////////
fb::scoped_screen_info::scoped_screen_info(asio::posix::stream_descriptor& fb) : fd{fb}
{
    command<FBIOGET_FSCREENINFO, void*> get_finfo{&finfo};
    fd.io_control(get_finfo);

    command<FBIOGET_VSCREENINFO, void*> get_vinfo{&vinfo};
    fd.io_control(get_vinfo);
    old_vinfo = vinfo;

    if (finfo.visual != FB_VISUAL_TRUECOLOR || vinfo.bits_per_pixel != 32)
    {
        info() << "Requesting 32-bit true-color mode";

        vinfo.bits_per_pixel = 32;
        update();

        // recheck again
        fd.io_control(get_finfo);
        fd.io_control(get_vinfo);

        if (finfo.visual != FB_VISUAL_TRUECOLOR)
            throw std::runtime_error{"Screen does not support true-color"};

        if (vinfo.bits_per_pixel != 32)
            throw std::runtime_error{"Screen does not support 32-bit colors"};
    }

    std::ostringstream os;
    os << vinfo.xres << "x" << vinfo.yres << " @ " << vinfo.bits_per_pixel << "bpp, ";

    if (vinfo.width > 0 && vinfo.height > 0)
    {
        os << vinfo.width << "mm x " << vinfo.height << "mm, ";

        double dpi_x = 25.4 * vinfo.xres / vinfo.width;
        double dpi_y = 25.4 * vinfo.yres / vinfo.height;
        dpi = (dpi_x + dpi_y) / 2 + .5;
    }
    os << dpi << " DPI";

    info() << "Screen info: " << os.str();
}

fb::scoped_screen_info::~scoped_screen_info()
{
    info() << "Restoring previous screen info";
    command<FBIOPUT_VSCREENINFO, void*> put_vinfo{&old_vinfo};
    fd.io_control(put_vinfo);
}

void fb::scoped_screen_info::update()
{
    command<FBIOPUT_VSCREENINFO, void*> put_vinfo{&vinfo};
    vinfo.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
    fd.io_control(put_vinfo);
}

////////////////////////////////////////////////////////////////////////////////
fb::scoped_mmap_ptr::scoped_mmap_ptr(asio::posix::stream_descriptor& fd, std::size_t size) : size{size}
{
    info() << "Mapping screen to memory";
    data = static_cast<decltype(data)>( mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.native_handle(), 0) );
    if (data == MAP_FAILED) throw posix_error{"mmap"};
}

fb::scoped_mmap_ptr::~scoped_mmap_ptr()
{
    info() << "Unmapping screen from memory";
    munmap(data, size);
}
