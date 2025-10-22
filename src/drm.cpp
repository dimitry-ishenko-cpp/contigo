////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "drm.hpp"
#include "error.hpp"

#include <fcntl.h> // open

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
