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
#include <xf86drmMode.h>

////////////////////////////////////////////////////////////////////////////////
namespace drm
{

namespace
{

////////////////////////////////////////////////////////////////////////////////
auto open(const asio::any_io_executor& ex, drm::num num)
{
    auto path = drm::path + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

auto get_res(asio::posix::stream_descriptor& fd)
{
    drm_res res{drmModeGetResources(fd.native_handle()), &drmModeFreeResources};
    if (!res) throw posix_error{"drmModeGetResources"};
    return res;
}

auto find_conn(asio::posix::stream_descriptor& fd, drm_res& res)
{
    auto conn_ids = std::span(res->connectors, res->count_connectors);
    for (auto&& conn_id : conn_ids)
    {
        drm_conn conn{drmModeGetConnector(fd.native_handle(), conn_id), &drmModeFreeConnector};
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes) return conn;
    }

    throw std::runtime_error{"No active connectors"};
}

auto get_mode(drm_conn& conn, unsigned n)
{
    return drm::mode{.idx = n,
        .width = conn->modes[n].hdisplay,
        .height= conn->modes[n].vdisplay,
        .rate  = conn->modes[n].vrefresh
    };
}

////////////////////////////////////////////////////////////////////////////////
auto get_enc(device& dev, std::uint32_t enc_id)
{
    return drm_enc{drmModeGetEncoder(dev.handle(), enc_id), &drmModeFreeEncoder};
}

auto find_crtc(device& dev)
{
    auto& res = dev.res();
    auto& conn = dev.conn();

    if (conn->encoder_id)
    {
        auto enc = get_enc(dev, conn->encoder_id);
        if (enc && enc->crtc_id) return enc->crtc_id;
    }

    auto enc_ids = std::span(conn->encoders, conn->count_encoders);
    for (auto&& enc_id : enc_ids)
        if (auto enc = get_enc(dev, enc_id))
            for (auto i = 0; i < res->count_crtcs; ++i)
                if (enc->possible_crtcs & (1 << i))
                    return res->crtcs[i];

    throw std::runtime_error{"No valid encoder+crtc combinations"};
}

auto get_crtc(device& dev, std::uint32_t crtc_id)
{
    auto crtc = drmModeGetCrtc(dev.handle(), crtc_id);
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
    res_{get_res(fd_)}, conn_{find_conn(fd_, res_)},
    mode_{get_mode(conn_, 0)}
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

////////////////////////////////////////////////////////////////////////////////
crtc::crtc(std::shared_ptr<device> dev) :
    dev_{std::move(dev)}, id_{find_crtc(*dev_)}, prev_{get_crtc(*dev_, id_)}
{ }

crtc::~crtc()
{
    auto& conn = dev_->conn();

    info() << "Restoring previous crtc";
    drmModeSetCrtc(dev_->handle(), prev_->crtc_id, prev_->buffer_id, prev_->x, prev_->y, &conn->connector_id, 1, &prev_->mode);
    drmModeFreeCrtc(prev_);
}

void crtc::activate(framebuf& fb)
{
    auto& conn = dev_->conn();
    auto& mode = dev_->mode();

    info() << "Activating crtc";
    auto code = drmModeSetCrtc(dev_->handle(), id_, fb.id(), 0, 0, &conn->connector_id, 1, &conn->modes[mode.idx]);
    if (code) throw posix_error{"drmModeSetCrtc"};
}

}
