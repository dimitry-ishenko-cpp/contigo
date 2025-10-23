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

#include <fcntl.h> // open
#include <xf86drm.h>
#include <xf86drmMode.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

inline auto open_device(const asio::any_io_executor& ex, drm::num num)
{
    auto path = drm::path + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

}

////////////////////////////////////////////////////////////////////////////////
drm::drm(const asio::any_io_executor& ex, drm::num num) :
    fd_{open_device(ex, num)}
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
