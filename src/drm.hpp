////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "geom.hpp"

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <cstdint>
#include <memory>

struct _drmModeRes;
using drm_mode_res = std::unique_ptr<_drmModeRes, void(*)(_drmModeRes*)>;

struct _drmModeConnector;
using drm_mode_conn = std::unique_ptr<_drmModeConnector, void(*)(_drmModeConnector*)>;

struct _drmModeEncoder;
using drm_mode_enc = std::unique_ptr<_drmModeEncoder, void(*)(_drmModeEncoder*)>;

struct _drmModeCrtc;

////////////////////////////////////////////////////////////////////////////////
class drm
{
public:
    ////////////////////
    static constexpr auto name = "card";
    static constexpr auto path = "/dev/dri/card";
    using num = unsigned;

    struct mode
    {
        struct dim dim;
        unsigned rate;
        unsigned dpi = 96;
    };

    ////////////////////
    drm(const asio::any_io_executor&, num);

    constexpr auto mode() const noexcept { return mode_; }

    void disable();
    void enable();

    ////////////////////
    static num any() { return 0; }

private:
    ////////////////////
    struct drm_scoped_crtc
    {
        asio::posix::stream_descriptor& fd;
        std::uint32_t id;
        std::uint32_t conn_id;
        _drmModeCrtc* old;

        drm_scoped_crtc(asio::posix::stream_descriptor&, drm_mode_res&, drm_mode_conn&);
        ~drm_scoped_crtc();
    };

    ////////////////////
    asio::posix::stream_descriptor fd_;

    drm_mode_res res_;
    drm_mode_conn conn_;
    drm_scoped_crtc crtc_;

    struct mode mode_;
};
