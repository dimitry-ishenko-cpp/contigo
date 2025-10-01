////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "tty.hpp"

#include <cerrno>
#include <system_error>
#include <type_traits>

#include <fcntl.h> // open()
#include <linux/vt.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

template<int Op, typename Type>
struct command
{
    Type val;
    constexpr auto data()
    {
        if constexpr (std::is_fundamental_v<Type>) return reinterpret_cast<void*>(val);
        else return static_cast<void*>(&val);
    }
    constexpr auto name() const { return Op; }
};

auto get_vt0(const asio::any_io_executor& ex)
{
    auto fd = ::open("/dev/tty0", O_RDWR);
    if (fd < 0) throw std::system_error{errno, std::system_category()};
    return asio::posix::stream_descriptor{ex, fd};
}

}

////////////////////////////////////////////////////////////////////////////////
tty::tty(const asio::any_io_executor& ex, unsigned num) :
    vt_{ex}, num_{num}
{ }

void tty::activate()
{
    auto vt0 = get_vt0(vt_.get_executor());

    command<VT_ACTIVATE, unsigned> activate{num_};
    vt0.io_control(activate);

    command<VT_WAITACTIVE, unsigned> wait_active{num_};
    vt0.io_control(wait_active);
}

unsigned tty::get_active(const asio::any_io_executor& ex)
{
    auto vt0 = get_vt0(ex);

    command<VT_GETSTATE, vt_stat> get_state;
    vt0.io_control(get_state);

    return get_state.val.v_active;
}
