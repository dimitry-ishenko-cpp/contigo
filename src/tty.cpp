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
#include <linux/kd.h>
#include <linux/vt.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

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

auto open(const asio::any_io_executor& ex, tty_num n)
{
    auto path = "/dev/tty" + std::to_string(n);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

}

////////////////////////////////////////////////////////////////////////////////
tty::tty(const asio::any_io_executor& ex, tty_num num, tty::action action) :
    num_{num}, vt_{open(ex, num_)}, active_{vt_, num, action}, state_{vt_}, mode_{vt_}
{ }

tty_num tty::active(const asio::any_io_executor& ex)
{
    auto tty0 = open(ex, 0);

    command<VT_GETSTATE, vt_stat> get_state;
    tty0.io_control(get_state);

    return get_state.val.v_active;
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_active::scoped_active(asio::posix::stream_descriptor& vt, tty_num num, tty::action action) :
    vt{vt}, old_num{tty::active(vt.get_executor())}
{
    if (num != old_num && action == tty::activate)
    {
        activate(num);
        active = true;
    }
}

tty::scoped_active::~scoped_active()
{
    if (active) activate(old_num);
}

void tty::scoped_active::activate(tty_num n)
{
    command<VT_ACTIVATE, tty_num> activate{n};
    vt.io_control(activate);

    command<VT_WAITACTIVE, tty_num> wait_active{n};
    vt.io_control(wait_active);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_raw_state::scoped_raw_state(asio::posix::stream_descriptor& vt) : vt{vt}
{
    if (tcgetattr(vt.native_handle(), &old_state) < 0) throw posix_error{"tcgetattr"};

    termios tio = old_state;
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(vt.native_handle(), TCSANOW, &tio) < 0) throw posix_error{"tcsetattr"};
}

tty::scoped_raw_state::~scoped_raw_state()
{
    tcsetattr(vt.native_handle(), TCSANOW, &old_state);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_graph_mode::scoped_graph_mode(asio::posix::stream_descriptor& vt) : vt{vt}
{
    command<KDGETMODE, unsigned*> get_mode{&old_mode};
    vt.io_control(get_mode);

    command<KDSETMODE, unsigned> set_graph{KD_GRAPHICS};
    vt.io_control(set_graph);
}

tty::scoped_graph_mode::~scoped_graph_mode()
{
    command<KDSETMODE, unsigned> set_mode{old_mode};
    std::error_code ec;
    vt.io_control(set_mode, ec);
}
