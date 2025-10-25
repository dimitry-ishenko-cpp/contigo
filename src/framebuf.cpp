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

framebuf_base::scoped_dumbuf::scoped_dumbuf(asio::posix::stream_descriptor& drm, dim dim, unsigned bits_per_pixel) : drm{drm}
{
    auto creq = drm_mode_create_dumb{
        .height = dim.height,
        .width = dim.width,
        .bpp = bits_per_pixel
    };
    command<DRM_IOCTL_MODE_CREATE_DUMB, drm_mode_create_dumb*> create{&creq};
    drm.io_control(create);

    handle = creq.handle;
    stride = creq.pitch;
    size = creq.size;
}

framebuf_base::scoped_dumbuf::~scoped_dumbuf()
{
    command<DRM_IOCTL_MODE_DESTROY_DUMB, drm_mode_destroy_dumb> dreq{handle};
    drm.io_control(dreq);
}

////////////////////////////////////////////////////////////////////////////////
framebuf_base::scoped_fbo::scoped_fbo(scoped_dumbuf& buf, dim dim, unsigned depth, unsigned bits_per_pixel) : drm{buf.drm}
{
    auto code = drmModeAddFB(drm.native_handle(), dim.width, dim.height, depth, bits_per_pixel, buf.stride, buf.handle, &id);
    if (code) throw posix_error{"drmModeAddFB"};
}

framebuf_base::scoped_fbo::~scoped_fbo() { drmModeRmFB(drm.native_handle(), id); }

////////////////////////////////////////////////////////////////////////////////
framebuf_base::scoped_mmap::scoped_mmap(scoped_dumbuf& buf) : size{buf.size}
{
    auto& drm = buf.drm;

    auto mreq = drm_mode_map_dumb{.handle = buf.handle};
    command<DRM_IOCTL_MODE_MAP_DUMB, drm_mode_map_dumb*> map{&mreq};
    drm.io_control(map);

    p = mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, drm.native_handle(), mreq.offset);
    if (p == MAP_FAILED) throw posix_error{"mmap"};
}

framebuf_base::scoped_mmap::~scoped_mmap() { munmap(p, size); }

////////////////////////////////////////////////////////////////////////////////
framebuf_base::framebuf_base(asio::posix::stream_descriptor& drm, dim dim, unsigned depth, unsigned bits_per_pixel) :
    buf_{drm, dim, bits_per_pixel}, fbo_{buf_, dim, depth, bits_per_pixel}, mmap_{buf_}
{
    info() << "Using framebuf: " << dim << "px, " << depth << "/" << bits_per_pixel <<  "-bit, stride=" << buf_.stride << ", size=" << buf_.size;
}
