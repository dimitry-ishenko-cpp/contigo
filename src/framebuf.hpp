////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "drm.hpp"
#include "pixman.hpp"

#include <asio/posix/stream_descriptor.hpp>
#include <cstdint>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

class scoped_dumbuf
{
    std::shared_ptr<device> dev_;
    std::uint32_t handle_;
    std::uint32_t stride_;
    std::size_t size_;

public:
    ////////////////////
    scoped_dumbuf(std::shared_ptr<device>, unsigned bits_per_pixel);
    ~scoped_dumbuf();

    constexpr auto handle() const noexcept { return handle_; }
    constexpr auto stride() const noexcept { return stride_; }
    constexpr auto size() const noexcept { return size_; }
};

class scoped_fbo
{
    std::shared_ptr<device> dev_;
    std::uint32_t id_;

public:
    ////////////////////
    scoped_fbo(std::shared_ptr<device>, scoped_dumbuf&, unsigned depth, unsigned bits_per_pixel);
    ~scoped_fbo();

    constexpr auto id() const noexcept { return id_; }
};

class scoped_mmapped_ptr
{
    void* data_;
    std::size_t size_;

public:
    ////////////////////
    scoped_mmapped_ptr(std::shared_ptr<device>, scoped_dumbuf&);
    ~scoped_mmapped_ptr();

    constexpr auto data() noexcept { return data_; }
};

////////////////////////////////////////////////////////////////////////////////
class framebuf
{
    std::shared_ptr<device> dev_;

    scoped_dumbuf buf_;
    scoped_fbo fbo_;
    scoped_mmapped_ptr mmap_;
    pixman::image image_;

public:
    ////////////////////
    explicit framebuf(std::shared_ptr<device>);

    constexpr auto id() const noexcept { return fbo_.id(); }

    void fill(int x, int y, const pixman::image& image) { image_.fill(x, y, image); }
    void commit();
};

////////////////////////////////////////////////////////////////////////////////
}
