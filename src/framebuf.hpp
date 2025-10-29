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

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

////////////////////////////////////////////////////////////////////////////////
class framebuf
{
public:
    ////////////////////
    explicit framebuf(device&, unsigned w, unsigned h);

    constexpr auto id() const noexcept { return fbo_.id; }
    auto& image() noexcept { return image_; }

    void commit();

private:
    ////////////////////
    struct scoped_dumbuf
    {
        asio::posix::stream_descriptor& drm;
        std::uint32_t handle;
        std::uint32_t stride;
        std::size_t size;

        scoped_dumbuf(asio::posix::stream_descriptor&, unsigned w, unsigned h);
        ~scoped_dumbuf();
    };

    struct scoped_fbo
    {
        asio::posix::stream_descriptor& drm;
        std::uint32_t id;

        scoped_fbo(asio::posix::stream_descriptor&, unsigned w, unsigned h, scoped_dumbuf&);
        ~scoped_fbo();
    };

    struct scoped_mapped_ptr
    {
        void* data;
        std::size_t size;

        scoped_mapped_ptr(asio::posix::stream_descriptor&, scoped_dumbuf&);
        ~scoped_mapped_ptr();
    };

    ////////////////////
    asio::posix::stream_descriptor& drm_;

    scoped_dumbuf buf_;
    scoped_fbo fbo_;
    scoped_mapped_ptr map_;

    pixman::image image_;
};

////////////////////////////////////////////////////////////////////////////////
}
