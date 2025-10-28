////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "command.hpp"
#include "drm.hpp"
#include "error.hpp"
#include "framebuf.hpp"
#include "logging.hpp"

#include <span>
#include <stdexcept>

#include <fcntl.h> // open
#include <xf86drm.h>

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

namespace
{

auto open(const asio::any_io_executor& ex, drm::num num)
{
    auto path = drm::path + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

auto get_resources(asio::posix::stream_descriptor& fd)
{
    resources ress{drmModeGetResources(fd.native_handle()), &drmModeFreeResources};
    if (!ress) throw posix_error{"drmModeGetResources"};
    return ress;
}

auto find_connector(asio::posix::stream_descriptor& fd, resources& ress)
{
    auto conn_ids = std::span(ress->connectors, ress->count_connectors);
    for (auto&& conn_id : conn_ids)
    {
        connector conn{drmModeGetConnector(fd.native_handle(), conn_id), &drmModeFreeConnector};
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes) return conn;
    }

    throw std::runtime_error{"No active connectors"};
}

auto get_mode(connector& conn, unsigned n)
{
    return mode{.idx = n,
        .width = conn->modes[n].hdisplay,
        .height= conn->modes[n].vdisplay,
        .rate  = conn->modes[n].vrefresh
    };
}

auto get_encoder(asio::posix::stream_descriptor& fd, std::uint32_t enc_id)
{
    return encoder{drmModeGetEncoder(fd.native_handle(), enc_id), &drmModeFreeEncoder};
}

auto find_crtc(asio::posix::stream_descriptor& fd, resources& ress, connector& conn)
{
    if (conn->encoder_id)
    {
        auto enc = get_encoder(fd, conn->encoder_id);
        if (enc && enc->crtc_id) return enc->crtc_id;
    }

    auto enc_ids = std::span(conn->encoders, conn->count_encoders);
    for (auto&& enc_id : enc_ids)
        if (auto enc = get_encoder(fd, enc_id))
            for (auto i = 0; i < ress->count_crtcs; ++i)
                if (enc->possible_crtcs & (1 << i))
                    return ress->crtcs[i];

    throw std::runtime_error{"No valid encoder+crtc combinations"};
}

auto get_crtc(asio::posix::stream_descriptor& fd, std::uint32_t crtc_id)
{
    auto crtc = drmModeGetCrtc(fd.native_handle(), crtc_id);
    if (!crtc) throw posix_error{"drmModeGetCrtc"};
    return crtc;
}

}

////////////////////////////////////////////////////////////////////////////////
num find()
{
    // TODO
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
device::device(const asio::any_io_executor& ex, drm::num num) : fd_{open(ex, num)},
    ress_{get_resources(fd_)},
    conn_{find_connector(fd_, ress_)}, mode_{get_mode(conn_, 0)},
    crtc_{fd_, ress_, conn_}
{
    std::string size;
    if (conn_->mmWidth && conn_->mmHeight)
    {
        size = std::to_string(conn_->mmWidth) + "mm x " + std::to_string(conn_->mmHeight) + "mm, ";

        double dpi_x = 25.4 * mode_.width / conn_->mmWidth;
        double dpi_y = 25.4 * mode_.height / conn_->mmHeight;
        mode_.dpi = (dpi_x + dpi_y) / 2 + .5;
    }

    info() << "Using screen: " << mode_.width << "x" << mode_.height << "@" << mode_.rate << "hz, " << size << "DPI=" << mode_.dpi;
}

void device::disable()
{
    info() << "Dropping drm master";
    command<DRM_IOCTL_DROP_MASTER, int> cmd{0};
    fd_.io_control(cmd);
}

void device::enable()
{
    info() << "Acquiring drm master";
    command<DRM_IOCTL_SET_MASTER, int> cmd{0};
    fd_.io_control(cmd);
}

void device::activate(framebuf& fb)
{
    info() << "Activating crtc";
    auto code = drmModeSetCrtc(fd_.native_handle(), crtc_.id, fb.id(), 0, 0, &conn_->connector_id, 1, &conn_->modes[mode_.idx]);
    if (code) throw posix_error{"drmModeSetCrtc"};
}

////////////////////////////////////////////////////////////////////////////////
device::crtc::crtc(asio::posix::stream_descriptor& fd, resources& res, connector& conn) :
    fd{fd}, id{find_crtc(fd, res, conn)}, conn_id{conn->connector_id}, prev{get_crtc(fd, id)}
{ }

device::crtc::~crtc()
{
    info() << "Restoring previous crtc";
    drmModeSetCrtc(fd.native_handle(), prev->crtc_id, prev->buffer_id, prev->x, prev->y, &conn_id, 1, &prev->mode);
    drmModeFreeCrtc(prev);
}

////////////////////////////////////////////////////////////////////////////////
}
