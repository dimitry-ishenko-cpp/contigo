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

auto get_drm_res(asio::posix::stream_descriptor& fd)
{
    drm_mode_res res{drmModeGetResources(fd.native_handle()), &drmModeFreeResources};
    if (!res) throw posix_error{"drmModeGetResources"};
    return res;
}

auto find_drm_conn(asio::posix::stream_descriptor& fd, drm_mode_res& res)
{
    auto conn_ids = std::span(res->connectors, res->count_connectors);
    for (auto&& conn_id : conn_ids)
    {
        drm_mode_conn conn{drmModeGetConnector(fd.native_handle(), conn_id), &drmModeFreeConnector};
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes) return conn;
    }

    throw std::runtime_error{"No active connectors"};
}

auto get_drm_enc(asio::posix::stream_descriptor& fd, std::uint32_t enc_id)
{
    return drm_mode_enc{drmModeGetEncoder(fd.native_handle(), enc_id), &drmModeFreeEncoder};
}

auto find_drm_crtc(asio::posix::stream_descriptor& fd, drm_mode_res& res, drm_mode_conn& conn)
{
    if (conn->encoder_id)
    {
        auto enc = get_drm_enc(fd, conn->encoder_id);
        if (enc && enc->crtc_id) return enc->crtc_id;
    }

    auto enc_ids = std::span(conn->encoders, conn->count_encoders);
    for (auto&& enc_id : enc_ids)
        if (auto enc = get_drm_enc(fd, enc_id))
            for (auto i = 0; i < res->count_crtcs; ++i)
                if (enc->possible_crtcs & (1 << i))
                    return res->crtcs[i];

    throw std::runtime_error{"No valid encoder+crtc combinations"};
}

auto get_drm_crtc(asio::posix::stream_descriptor& fd, std::uint32_t crtc_id)
{
    auto crtc = drmModeGetCrtc(fd.native_handle(), crtc_id);
    if (!crtc) throw posix_error{"drmModeGetCrtc"};
    return crtc;
}

}

////////////////////////////////////////////////////////////////////////////////
drm::drm(const asio::any_io_executor& ex, drm::num num) :
    fd_{open_device(ex, num)},
    res_{get_drm_res(fd_)}, conn_{find_drm_conn(fd_, res_)}, crtc_{fd_, res_, conn_}
{
    mode_ = dim{conn_->modes[0].hdisplay, conn_->modes[0].vdisplay};

    std::string size;
    if (conn_->mmWidth && conn_->mmHeight)
    {
        size = std::to_string(conn_->mmWidth) + "mm x " + std::to_string(conn_->mmHeight) + "mm, ";

        double dpi_x = 25.4 * mode_.width / conn_->mmWidth;
        double dpi_y = 25.4 * mode_.height / conn_->mmHeight;
        dpi_ = (dpi_x + dpi_y) / 2 + .5;
    }

    info() << "Using screen: " << mode_ << ", " << size << "DPI = " << dpi_;
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
drm::drm_scoped_crtc::drm_scoped_crtc(asio::posix::stream_descriptor& fd, drm_mode_res& res, drm_mode_conn& conn) :
    fd{fd}, id{find_drm_crtc(fd, res, conn)}, conn_id{conn->connector_id}, old{get_drm_crtc(fd, id)}
{ }

drm::drm_scoped_crtc::~drm_scoped_crtc()
{
    info() << "Restoring previous crtc";
    drmModeSetCrtc(fd.native_handle(), old->crtc_id, old->buffer_id, old->x, old->y, &conn_id, 1, &old->mode);
    drmModeFreeCrtc(old);
}
