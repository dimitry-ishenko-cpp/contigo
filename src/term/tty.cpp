////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "logging.hpp"
#include "tty.hpp"

#include <asio.hpp>
#include <type_traits>

#include <fcntl.h> // open
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

auto open(const asio::any_io_executor& ex, tty::num num)
{
    auto path = "/dev/tty" + std::to_string(num);

    auto fd = ::open(path.data(), O_RDWR);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

enum signals
{
    release = SIGUSR1,
    acquire = SIGUSR2,
};

}

////////////////////////////////////////////////////////////////////////////////
tty::tty(const asio::any_io_executor& ex, tty::num num, bool activate) :
    fd_{open(ex, num)}, sigs_{ex, release, acquire},
    active_{fd_, num, activate}, raw_{fd_}, process_{fd_}, graphic_{fd_}
{
    sched_signal_callback();
    sched_async_read();
}

tty::num tty::active(const asio::any_io_executor& ex)
{
    auto tty0 = open(ex, 0);

    command<VT_GETSTATE, vt_stat> get_state;
    tty0.io_control(get_state);

    return get_state.val.v_active;
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_active::scoped_active(asio::posix::stream_descriptor& vt, tty::num num, bool activate) :
    fd{vt}, old_num{tty::active(vt.get_executor())}
{
    if (num != old_num && activate)
    {
        make_active(num);
        active = true;
    }
}

tty::scoped_active::~scoped_active()
{
    if (active) make_active(old_num);
}

void tty::scoped_active::make_active(tty::num num)
{
    info() << "Activating tty" << num;
    command<VT_ACTIVATE, tty::num> activate{num};
    fd.io_control(activate);

    command<VT_WAITACTIVE, tty::num> wait_active{num};
    fd.io_control(wait_active);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_raw_state::scoped_raw_state(asio::posix::stream_descriptor& vt) : fd{vt}
{
    if (tcgetattr(vt.native_handle(), &old_state) < 0) throw posix_error{"tcgetattr"};

    info() << "Switching to raw state";
    termios tio = old_state;
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(vt.native_handle(), TCSANOW, &tio) < 0) throw posix_error{"tcsetattr"};
}

tty::scoped_raw_state::~scoped_raw_state()
{
    info() << "Restoring previous state";
    tcsetattr(fd.native_handle(), TCSANOW, &old_state);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_process_mode::scoped_process_mode(asio::posix::stream_descriptor& vt) : fd{vt}
{
    command<VT_SETMODE, vt_mode> mode{{
        .mode  = VT_PROCESS,
        .waitv = 0,
        .relsig= release,
        .acqsig= acquire,
        .frsig = 0,
    }};
    fd.io_control(mode);
}

tty::scoped_process_mode::~scoped_process_mode()
{
    command<VT_SETMODE, vt_mode> mode{{
        .mode = VT_AUTO,
        .waitv = 0,
        .relsig= 0,
        .acqsig= 0,
        .frsig = 0,
    }};
    fd.io_control(mode);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_graphic_mode::scoped_graphic_mode(asio::posix::stream_descriptor& vt) : fd{vt}
{
    command<KDGETMODE, unsigned*> get_mode{&old_mode};
    vt.io_control(get_mode);

    info() << "Switching to graphic mode";
    command<KDSETMODE, unsigned> set_graph{KD_GRAPHICS};
    vt.io_control(set_graph);
}

tty::scoped_graphic_mode::~scoped_graphic_mode()
{
    info() << "Restoring previous mode";
    command<KDSETMODE, unsigned> set_mode{old_mode};
    std::error_code ec;
    fd.io_control(set_mode, ec);
}

////////////////////////////////////////////////////////////////////////////////
void tty::sched_signal_callback()
{
    sigs_.async_wait([&](std::error_code ec, int signal)
    {
        if (!ec)
        {
            command<VT_RELDISP, int> rel;
            switch (signal)
            {
            case release:
                if (release_cb_) release_cb_();

                rel.val = 1;
                fd_.io_control(rel);
                break;

            case acquire:
                rel.val = VT_ACKACQ;
                fd_.io_control(rel);

                if (acquire_cb_) acquire_cb_();
                break;
            }

            sched_signal_callback();
        }
    });
}

void tty::sched_async_read()
{
    fd_.async_read_some(asio::buffer(buffer_), [&](std::error_code ec, std::size_t size)
    {
        if (!ec)
        {
            if (read_cb_) read_cb_(std::span<const char>{buffer_.begin(), size});
            sched_async_read();
        }
    });
}
