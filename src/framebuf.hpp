////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"
#include "drm.hpp"
#include "geom.hpp"
#include "image.hpp"

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
    constexpr auto data() const noexcept { return data_; }
};

////////////////////////////////////////////////////////////////////////////////
class framebuf_base
{
protected:
    ////////////////////
    std::shared_ptr<device> dev_;

    scoped_dumbuf buf_;
    scoped_fbo fbo_;
    scoped_mmapped_ptr mmap_;

    framebuf_base(std::shared_ptr<device>, unsigned depth, unsigned bits_per_pixel);

public:
    ////////////////////
    constexpr auto id() const noexcept { return fbo_.id(); }

    constexpr auto data() noexcept { return mmap_.data(); }
    constexpr auto data() const noexcept { return mmap_.data(); }

    void commit();
};

template<typename C>
struct wrapped_ptr
{
    framebuf_base* fb;
    using element_type = C;

    constexpr auto get() noexcept { return static_cast<C*>(fb->data()); }
    constexpr auto get() const noexcept { return static_cast<const C*>(fb->data()); }
};

template<typename C>
class framebuf : public framebuf_base, public image_base<wrapped_ptr<C>>
{
public:
    ////////////////////
    explicit framebuf(std::shared_ptr<device> dev) :
        framebuf_base{std::move(dev), depth<C>, bits_per_pixel<C>},
        image_base<wrapped_ptr<C>>{dev_->mode().dim, buf_.stride(), wrapped_ptr<C>{this}}
    { }

    template<typename D>
    void fill(pos pos, const image_base<D>& src)
        requires std::same_as<C, typename image_base<D>::color_type>
    {
        // TODO track damage
        ::fill(*this, pos, src);
    }
};

////////////////////////////////////////////////////////////////////////////////
}
