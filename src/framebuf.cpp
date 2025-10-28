////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "command.hpp"
#include "error.hpp"
#include "framebuf.hpp"
#include "logging.hpp"

#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

////////////////////////////////////////////////////////////////////////////////
framebuf::framebuf(device& dev, unsigned w, unsigned h) : drm_{dev.fd_},
    buf_{drm_, w, h},
    fbo_{drm_, w, h, buf_},
    map_{drm_, buf_},
    image_{w, h, buf_.stride, map_.data}
{
    info() << "Using framebuf: " << image_.depth << "-bit color, " << image_.bits_per_pixel <<  " bpp, stride=" << buf_.stride << ", size=" << buf_.size;
}

void framebuf::commit()
{
    auto code = drmModeDirtyFB(drm_.native_handle(), fbo_.id, nullptr, 0);
    if (code) throw posix_error{"drmModeDirtyFB"};
}

framebuf::scoped_dumbuf::scoped_dumbuf(asio::posix::stream_descriptor& drm, unsigned w, unsigned h) : drm{drm}
{
    command<DRM_IOCTL_MODE_CREATE_DUMB, drm_mode_create_dumb> create{{
        .height = h,
        .width = w,
        .bpp = pixman::image::bits_per_pixel
    }};
    drm.io_control(create);

    handle = create.val.handle;
    stride = create.val.pitch;
    size = create.val.size;
}

framebuf::scoped_dumbuf::~scoped_dumbuf()
{
    command<DRM_IOCTL_MODE_DESTROY_DUMB, drm_mode_destroy_dumb> destroy{handle};
    drm.io_control(destroy);
}

////////////////////////////////////////////////////////////////////////////////
framebuf::scoped_fbo::scoped_fbo(asio::posix::stream_descriptor& drm, unsigned w, unsigned h, scoped_dumbuf& buf) : drm{drm}
{
    auto code = drmModeAddFB(drm.native_handle(), w, h,
        pixman::image::depth, pixman::image::bits_per_pixel, buf.stride, buf.handle, &id
    );
    if (code) throw posix_error{"drmModeAddFB"};
}

framebuf::scoped_fbo::~scoped_fbo() { drmModeRmFB(drm.native_handle(), id); }

////////////////////////////////////////////////////////////////////////////////
framebuf::scoped_mapped_ptr::scoped_mapped_ptr(asio::posix::stream_descriptor& drm, scoped_dumbuf& buf) : size{buf.size}
{
    command<DRM_IOCTL_MODE_MAP_DUMB, drm_mode_map_dumb> map{{
        .handle = buf.handle
    }};
    drm.io_control(map);

    data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, drm.native_handle(), map.val.offset);
    if (data == MAP_FAILED) throw posix_error{"mmap"};
}

framebuf::scoped_mapped_ptr::~scoped_mapped_ptr() { munmap(data, size); }

////////////////////////////////////////////////////////////////////////////////
}
