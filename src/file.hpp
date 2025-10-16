////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <string_view>

#include <fcntl.h> // open

inline auto open(const asio::any_io_executor& ex, std::string_view path)
{
    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}
