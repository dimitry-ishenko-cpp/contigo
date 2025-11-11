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

#include <filesystem>
#include <span>
#include <stdexcept>

#include <fcntl.h> // open
#include <xf86drm.h>

namespace fs = std::filesystem;

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

auto get_connector(asio::posix::stream_descriptor& fd, std::uint32_t conn_id)
{
    return connector{drmModeGetConnector(fd.native_handle(), conn_id), &drmModeFreeConnector};
}

auto get_encoder(asio::posix::stream_descriptor& fd, std::uint32_t enc_id)
{
    return encoder{drmModeGetEncoder(fd.native_handle(), enc_id), &drmModeFreeEncoder};
}

mode get_mode(const connector& conn, unsigned n)
{
    return {
        .idx = n,
        .width = conn->modes[n].hdisplay,
        .height= conn->modes[n].vdisplay,
        .rate  = conn->modes[n].vrefresh
    };
}

auto get_name(const connector& conn)
{
    static constexpr const char* types[] =
    {
        "Unknown",
        "VGA",
        "DVI-I", "DVI-D", "DVI-A",
        "Composite", "S-Video",
        "LVDS",
        "Component",
        "9PinDIN",
        "DP",
        "HDMI-A", "HDMI-B",
        "TV",
        "eDP",
        "Virtual",
        "DSI", "DPI", "Writeback", "SPI",
        "USB"
    };

    std::string type = types[conn->connector_type < std::size(types) ? conn->connector_type : 0];
    type += "-" + std::to_string(conn->connector_type_id);

    return type;
}

template<unsigned Op, typename T>
inline void drm_control(asio::posix::stream_descriptor& fd, command<Op, T>& cmd)
{
    std::error_code ec;

    do fd.io_control(cmd, ec);
    while (ec == std::errc::interrupted || ec == std::errc::resource_unavailable_try_again);

    if (ec) throw std::system_error{ec};
}

////////////////////////////////////////////////////////////////////////////////
auto find_connector(asio::posix::stream_descriptor& fd, const resources& ress)
{
    auto conn_ids = std::span(ress->connectors, ress->count_connectors);
    for (auto conn_id : conn_ids)
    {
        auto conn = get_connector(fd, conn_id);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes) return conn;
    }

    throw std::runtime_error{"Connector not found"};
}

auto find_crtc(asio::posix::stream_descriptor& fd, const resources& ress, const connector& conn)
{
    if (conn->encoder_id)
    {
        auto enc = get_encoder(fd, conn->encoder_id);
        if (enc && enc->crtc_id) return enc->crtc_id;
    }

    auto enc_ids = std::span(conn->encoders, conn->count_encoders);
    for (auto enc_id : enc_ids)
        if (auto enc = get_encoder(fd, enc_id))
            for (auto i = 0; i < ress->count_crtcs; ++i)
                if (enc->possible_crtcs & (1 << i))
                    return ress->crtcs[i];

    throw std::runtime_error{"Suitable encoder+crtc combo not found"};
}

}

////////////////////////////////////////////////////////////////////////////////
num find()
{
    for (auto n = 0; n < 16; ++n)
    {
        auto path = drm::path + std::to_string(n);
        if (fs::exists(path)) return n;
    }
    throw std::runtime_error{"Graphics card not found"};
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

    info() << "Screen info: " << mode_.width << "x" << mode_.height << "@" << mode_.rate << "hz, " << size << "DPI=" << mode_.dpi;

    sched_vblank_wait();
}

void device::acquire_master()
{
    info() << "Acquiring drm master";
    command<DRM_IOCTL_SET_MASTER, int> set_master{};
    drm_control(fd_, set_master);
}

void device::drop_master()
{
    info() << "Dropping drm master";
    command<DRM_IOCTL_DROP_MASTER, int> drop_master{};
    drm_control(fd_, drop_master);
}

void device::sched_vblank_wait()
{
    drmVBlank vbl{ .request = {
        .type = static_cast<drmVBlankSeqType>(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT | DRM_VBLANK_NEXTONMISS),
        .sequence = 1
    }};
    auto code = drmWaitVBlank(fd_.native_handle(), &vbl);
    if (code) throw posix_error{"drmModeSetCrtc"};

    static drmEventContext ctx{
        .version = DRM_EVENT_CONTEXT_VERSION
    };

    fd_.async_wait(fd_.wait_read, [&](std::error_code ec)
    {
        if (!ec)
        {
            drmHandleEvent(fd_.native_handle(), &ctx);
            if (vblank_cb_) vblank_cb_();

            sched_vblank_wait();
        }
    });
}

////////////////////////////////////////////////////////////////////////////////
device::crtc::crtc(asio::posix::stream_descriptor& fd, const resources& ress, const connector& conn) : fd{fd}
{
    auto crtc_id = find_crtc(fd, ress, conn);

    dev = drmModeGetCrtc(fd.native_handle(), crtc_id);
    if (!dev) throw posix_error{"drmModeGetCrtc"};

    conns.push_back(conn->connector_id);

    info() << "Outputting to: " << get_name(conn);
}

device::crtc::~crtc()
{
    info() << "Restoring previous crtc";
    drmModeSetCrtc(fd.native_handle(), dev->crtc_id, dev->buffer_id, dev->x, dev->y, conns.data(), conns.size(), &dev->mode);
    drmModeFreeCrtc(dev);
}

void device::crtc::set(framebuf& fb, drmModeModeInfo& mode)
{
    info() << "Setting up crtc";
    auto code = drmModeSetCrtc(fd.native_handle(), dev->crtc_id, fb.id(), 0, 0, conns.data(), conns.size(), &mode);
    if (code) throw posix_error{"drmModeSetCrtc"};
}

////////////////////////////////////////////////////////////////////////////////
}
