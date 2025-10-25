////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "color.hpp"
#include "geom.hpp"

#include <asio/posix/stream_descriptor.hpp>
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
class framebuf_base
{
protected:
    ////////////////////
    struct scoped_dumbuf
    {
        asio::posix::stream_descriptor& drm;
        std::uint32_t handle;
        std::uint32_t stride;
        std::size_t size;

        scoped_dumbuf(asio::posix::stream_descriptor&, dim, unsigned bits_per_pixel);
        ~scoped_dumbuf();
    };

    struct scoped_fbo
    {
        asio::posix::stream_descriptor& drm;
        std::uint32_t id;

        scoped_fbo(scoped_dumbuf&, dim, unsigned depth, unsigned bits_per_pixel);
        ~scoped_fbo();
    };

    struct scoped_mmap
    {
        void* p;
        std::size_t size;

        scoped_mmap(scoped_dumbuf&);
        ~scoped_mmap();
    };

    scoped_dumbuf buf_;
    scoped_fbo fbo_;
    scoped_mmap mmap_;

    ////////////////////
    framebuf_base(asio::posix::stream_descriptor&, dim, unsigned depth, unsigned bits_per_pixel);
};

template<typename C>
class framebuf : public framebuf_base
{
public:
    ////////////////////
    using element_type = C;

    framebuf(asio::posix::stream_descriptor& drm, dim dim) :
        framebuf_base{drm, dim, depth<C>, bits_per_pixel<C>}
    { }

    constexpr auto stride() const noexcept { return buf_.stride; }
    constexpr auto id() const noexcept { return fbo_.id; }

    auto get() noexcept { return static_cast<C*>(mmap_.p); }
    auto get() const noexcept { return static_cast<C*>(mmap_.p); }
};
