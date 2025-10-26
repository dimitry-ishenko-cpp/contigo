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

scoped_dumbuf::scoped_dumbuf(std::shared_ptr<device> dev, unsigned bits_per_pixel) :
    dev_{std::move(dev)}
{
    auto dim = dev_->mode().dim;
    command<DRM_IOCTL_MODE_CREATE_DUMB, drm_mode_create_dumb> create{{
        .height = dim.height,
        .width = dim.width,
        .bpp = bits_per_pixel
    }};
    dev_->io_control(create);

    handle_ = create.val.handle;
    stride_ = create.val.pitch;
    size_ = create.val.size;
}

scoped_dumbuf::~scoped_dumbuf()
{
    command<DRM_IOCTL_MODE_DESTROY_DUMB, drm_mode_destroy_dumb> destroy{handle_};
    dev_->io_control(destroy);
}

////////////////////////////////////////////////////////////////////////////////
scoped_fbo::scoped_fbo(std::shared_ptr<device> dev, scoped_dumbuf& buf, unsigned depth, unsigned bits_per_pixel) :
    dev_{std::move(dev)}
{
    auto dim = dev_->mode().dim;
    auto code = drmModeAddFB(dev_->handle(), dim.width, dim.height, depth, bits_per_pixel, buf.stride(), buf.handle(), &id_);
    if (code) throw posix_error{"drmModeAddFB"};
}

scoped_fbo::~scoped_fbo() { drmModeRmFB(dev_->handle(), id_); }

////////////////////////////////////////////////////////////////////////////////
scoped_mmapped_ptr::scoped_mmapped_ptr(std::shared_ptr<device> dev, scoped_dumbuf& buf) :
    size_{buf.size()}
{
    command<DRM_IOCTL_MODE_MAP_DUMB, drm_mode_map_dumb> map{{
        .handle = buf.handle()
    }};
    dev->io_control(map);

    data_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, dev->handle(), map.val.offset);
    if (data_ == MAP_FAILED) throw posix_error{"mmap"};
}

scoped_mmapped_ptr::~scoped_mmapped_ptr() { munmap(data_, size_); }

////////////////////////////////////////////////////////////////////////////////
framebuf_base::framebuf_base(std::shared_ptr<device> dev, unsigned depth, unsigned bits_per_pixel) :
    dev_{std::move(dev)}, buf_{dev_, bits_per_pixel}, fbo_{dev_, buf_, depth, bits_per_pixel}, mmap_{dev_, buf_}
{
    info() << "Using framebuf: " << depth << "-bit color, " << bits_per_pixel <<  " bpp, stride=" << buf_.stride() << ", size=" << buf_.size();
}

void framebuf_base::commit()
{
    auto code = drmModeDirtyFB(dev_->handle(), fbo_.id(), nullptr, 0);
    if (code) throw posix_error{"drmModeDirtyFB"};
}

////////////////////////////////////////////////////////////////////////////////
}
