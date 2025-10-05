////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "tty.hpp"

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

auto open_vt(const asio::any_io_executor& ex, unsigned num)
{
    auto name = "/dev/tty" + std::to_string(num);

    auto fd = ::open(name.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open_vt"};

    return asio::posix::stream_descriptor{ex, fd};
}

unsigned active_vt(const asio::any_io_executor& ex)
{
    auto vt0 = open_vt(ex, 0);

    command<VT_GETSTATE, vt_stat> get_state;
    vt0.io_control(get_state);

    return get_state.val.v_active;
}

}

////////////////////////////////////////////////////////////////////////////////
tty::tty(const asio::any_io_executor& ex, std::optional<unsigned> num, action action) :
    num_{num.value_or(active_vt(ex))}, vt_{open_vt(ex, num_)}
{
    if (action == activate)
    {
        command<VT_ACTIVATE, unsigned> activate{num_};
        vt_.io_control(activate);

        command<VT_WAITACTIVE, unsigned> wait_active{num_};
        vt_.io_control(wait_active);
    }
}
