////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <cstdint>
#include <memory>

#include <xf86drmMode.h>

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

constexpr auto name = "card";
constexpr auto path = "/dev/dri/card";
using num = unsigned;

num find();

struct mode
{
    unsigned idx;
    unsigned width, height;
    unsigned rate;
    unsigned dpi = 96;
};

class framebuf;

using resources = std::unique_ptr<drmModeRes, void(*)(drmModeRes*)>;
using connector = std::unique_ptr<drmModeConnector, void(*)(drmModeConnector*)>;
using encoder = std::unique_ptr<drmModeEncoder, void(*)(drmModeEncoder*)>;

////////////////////////////////////////////////////////////////////////////////
class device
{
public:
    ////////////////////
    device(const asio::any_io_executor&, num);

    constexpr auto& mode() const noexcept { return mode_; }

    void disable();
    void enable();

    void activate(framebuf&);

private:
    ////////////////////
    struct crtc
    {
        asio::posix::stream_descriptor& fd;
        std::uint32_t id, conn_id;
        drmModeCrtc* prev;

        crtc(asio::posix::stream_descriptor&, resources&, connector&);
        ~crtc();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;

    resources ress_;
    connector conn_;
    drm::mode mode_;
    crtc crtc_;

    friend class framebuf;
};

////////////////////////////////////////////////////////////////////////////////
}
