////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "io.hpp"
#include "logging.hpp"
#include "tty.hpp"

#include <asio.hpp>
#include <string>

#include <linux/kd.h>
#include <linux/vt.h>

////////////////////////////////////////////////////////////////////////////////
namespace
{

auto tty_path(tty::num num) { return "/dev/tty" + std::to_string(num); }

enum signals
{
    release = SIGUSR1,
    acquire = SIGUSR2,
};

}

////////////////////////////////////////////////////////////////////////////////
tty::tty(const asio::any_io_executor& ex, tty::num num, bool activate) :
    fd_{open(ex, tty_path(num))}, sigs_{ex, release, acquire},
    active_{fd_, num, activate}, attrs_{fd_}, vt_mode_{fd_}, kd_mode_{fd_}
{
    sched_signal_callback();
    sched_async_read();
}

tty::num tty::active(const asio::any_io_executor& ex)
{
    auto tty0 = open(ex, tty_path(0));

    command<VT_GETSTATE, vt_stat> get_state;
    tty0.io_control(get_state);

    return get_state.val.v_active;
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_active_vt::scoped_active_vt(asio::posix::stream_descriptor& vt, tty::num num, bool activate) :
    fd{vt}, old_num{tty::active(vt.get_executor())}
{
    if (num != old_num && activate)
    {
        make_active(num);
        active = true;
    }
}

tty::scoped_active_vt::~scoped_active_vt()
{
    if (active) make_active(old_num);
}

void tty::scoped_active_vt::make_active(tty::num num)
{
    info() << "Activating tty" << num;
    command<VT_ACTIVATE, tty::num> activate{num};
    fd.io_control(activate);

    command<VT_WAITACTIVE, tty::num> wait_active{num};
    fd.io_control(wait_active);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_attrs::scoped_attrs(asio::posix::stream_descriptor& vt) : fd{vt}
{
    if (tcgetattr(vt.native_handle(), &old_attrs) < 0) throw posix_error{"tcgetattr"};

    info() << "Switching tty to raw mode";
    termios tio = old_attrs;
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(vt.native_handle(), TCSANOW, &tio) < 0) throw posix_error{"tcsetattr"};
}

tty::scoped_attrs::~scoped_attrs()
{
    info() << "Restoring tty attrs";
    tcsetattr(fd.native_handle(), TCSANOW, &old_attrs);
}

////////////////////////////////////////////////////////////////////////////////
tty::scoped_vt_mode::scoped_vt_mode(asio::posix::stream_descriptor& vt) : fd{vt}
{
    info() << "Enabling process switch mode";
    command<VT_SETMODE, vt_mode> mode{{
        .mode  = VT_PROCESS,
        .waitv = 0,
        .relsig= release,
        .acqsig= acquire,
        .frsig = 0,
    }};
    fd.io_control(mode);
}

tty::scoped_vt_mode::~scoped_vt_mode()
{
    info() << "Restoring auto switch mode";
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
tty::scoped_kd_mode::scoped_kd_mode(asio::posix::stream_descriptor& vt) : fd{vt}
{
    command<KDGETMODE, unsigned*> get_mode{&old_mode};
    vt.io_control(get_mode);

    info() << "Switching to graphics mode";
    command<KDSETMODE, unsigned> set_graph{KD_GRAPHICS};
    vt.io_control(set_graph);
}

tty::scoped_kd_mode::~scoped_kd_mode()
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
