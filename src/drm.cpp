////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "command.hpp"
#include "drm.hpp"
#include "error.hpp"
#include "logging.hpp"

#include <span>
#include <stdexcept>

#include <fcntl.h> // open
#include <xf86drm.h>
#include <xf86drmMode.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

auto open_device(const asio::any_io_executor& ex, drm::num num)
{
    auto path = drm::path + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

auto get_drm_mode_res(asio::posix::stream_descriptor& fd)
{
    drm_mode_res res{drmModeGetResources(fd.native_handle()), &drmModeFreeResources};
    if (!res) throw posix_error{"drmModeGetResources"};
    return res;
}

auto get_drm_mode_conn(asio::posix::stream_descriptor& fd, drm_mode_res& res)
{
    auto conn_ids = std::span(res->connectors, res->count_connectors);
    for (auto&& conn_id : conn_ids)
    {
        drm_mode_conn conn{drmModeGetConnector(fd.native_handle(), conn_id), &drmModeFreeConnector};
        if (!conn) throw posix_error{"drmModeGetConnector"};
        if (conn->connection == DRM_MODE_CONNECTED) return conn;
    }

    throw std::runtime_error{"No active connectors"};
}

auto get_drm_mode_enc(asio::posix::stream_descriptor& fd, drm_mode_conn& conn)
{
    drm_mode_enc enc{drmModeGetEncoder(fd.native_handle(), conn->encoder_id), &drmModeFreeEncoder};
    if (!enc) throw posix_error{"drmModeGetEncoder"};
    return enc;
}

}

////////////////////////////////////////////////////////////////////////////////
drm::drm(const asio::any_io_executor& ex, drm::num num) :
    fd_{open_device(ex, num)},
    res_{get_drm_mode_res(fd_)}, conn_{get_drm_mode_conn(fd_, res_)}, enc_{get_drm_mode_enc(fd_, conn_)},
    crtc_{fd_, conn_, enc_}
{
    // TODO
}

void drm::disable()
{
    info() << "Dropping drm master";
    command<DRM_IOCTL_DROP_MASTER, int> cmd{0};
    fd_.io_control(cmd);
}

void drm::enable()
{
    info() << "Acquiring drm master";
    command<DRM_IOCTL_SET_MASTER, int> cmd{0};
    fd_.io_control(cmd);
}

////////////////////////////////////////////////////////////////////////////////
drm::drm_scoped_crtc::drm_scoped_crtc(asio::posix::stream_descriptor& drm, drm_mode_conn& conn, drm_mode_enc& enc) :
    fd{drm}, crtc{drmModeGetCrtc(fd.native_handle(), enc->crtc_id)}, conn_id{conn->connector_id}
{
    if (!crtc) throw posix_error{"drmModeGetCrtc"};

    // TODO set up crtc
}

drm::drm_scoped_crtc::~drm_scoped_crtc()
{
    info() << "Restoring previous crtc";
    drmModeSetCrtc(fd.native_handle(), crtc->crtc_id, crtc->buffer_id, crtc->x, crtc->y, &conn_id, 1, &crtc->mode);
    drmModeFreeCrtc(crtc);
}
