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

struct _drmModeRes;
using drm_res = std::unique_ptr<_drmModeRes, void(*)(_drmModeRes*)>;

struct _drmModeConnector;
using drm_conn = std::unique_ptr<_drmModeConnector, void(*)(_drmModeConnector*)>;

struct _drmModeEncoder;
using drm_enc = std::unique_ptr<_drmModeEncoder, void(*)(_drmModeEncoder*)>;

struct _drmModeCrtc;

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

////////////////////////////////////////////////////////////////////////////////
class device
{
    asio::posix::stream_descriptor fd_;

    drm_res res_;
    drm_conn conn_;
    drm::mode mode_;

public:
    ////////////////////
    device(const asio::any_io_executor&, num);

    void disable();
    void enable();

    auto handle() { return fd_.native_handle(); }
    template<typename Cmd> void io_control(Cmd& cmd) { fd_.io_control(cmd); }

    constexpr auto& res() { return res_; }
    constexpr auto& conn() { return conn_; }

    constexpr auto& mode() const noexcept { return mode_; }
};

////////////////////////////////////////////////////////////////////////////////
class crtc
{
    std::shared_ptr<device> dev_;
    std::uint32_t id_;
    _drmModeCrtc* prev_;

public:
    ////////////////////
    explicit crtc(std::shared_ptr<device>);
    ~crtc();

    void activate(framebuf&);
};

////////////////////////////////////////////////////////////////////////////////
}
