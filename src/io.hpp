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
#include <type_traits>

#include <fcntl.h> // open

////////////////////////////////////////////////////////////////////////////////
/**
 * @struct command
 * @brief command template for use with asio io_control()
 */
template<int Op, typename T>
struct command
{
    T val;
    constexpr auto data()
    {
             if constexpr (std::is_fundamental_v<T>) return reinterpret_cast<void*>(val);
        else if constexpr (std::is_pointer_v<T>) return static_cast<void*>(val);
        else return static_cast<void*>(&val);
    }
    constexpr auto name() const { return Op; }
};

inline auto open(const asio::any_io_executor& ex, std::string_view path)
{
    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}
