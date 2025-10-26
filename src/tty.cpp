////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "command.hpp"
#include "error.hpp"
#include "logging.hpp"
#include "tty.hpp"

#include <asio/buffer.hpp>
#include <string>

#include <fcntl.h> // open
#include <linux/kd.h>
#include <linux/vt.h>

////////////////////////////////////////////////////////////////////////////////
namespace tty
{

////////////////////////////////////////////////////////////////////////////////
namespace
{

auto open(const asio::any_io_executor& ex, tty::num num)
{
    auto path = tty::path + std::to_string(num);

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
num active(const asio::any_io_executor& ex)
{
    auto tty0 = tty::open(ex, 0);

    command<VT_GETSTATE, vt_stat> get_state;
    tty0.io_control(get_state);

    return get_state.val.v_active;
}

////////////////////////////////////////////////////////////////////////////////
device::device(const asio::any_io_executor& ex, tty::num num) : fd_{open(ex, num)}
{
    sched_async_read();
}

void device::sched_async_read()
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

////////////////////////////////////////////////////////////////////////////////
scoped_active::scoped_active(std::shared_ptr<device> dev, tty::num num, bool activate) :
    dev_{std::move(dev)}, prev_{tty::active(dev_->executor())}, num_{num}
{
    if (num_ != prev_ && activate)
    {
        make_active(num_);
        active_ = true;
    }
}

scoped_active::~scoped_active()
{
    if (active_)
    {
        auto num = active(dev_->executor());
        if (num == num_) make_active(prev_);
    }
}

void scoped_active::make_active(tty::num num)
{
    info() << "Activating tty" << num;
    command<VT_ACTIVATE, tty::num> activate{num};
    dev_->io_control(activate);

    command<VT_WAITACTIVE, tty::num> wait_active{num};
    dev_->io_control(wait_active);
}

////////////////////////////////////////////////////////////////////////////////
scoped_raw_mode::scoped_raw_mode(std::shared_ptr<device> dev) : dev_{std::move(dev)}
{
    if (tcgetattr(dev_->handle(), &prev_) < 0) throw posix_error{"tcgetattr"};

    info() << "Switching tty to raw mode";
    termios tio = prev_;
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(dev_->handle(), TCSANOW, &tio) < 0) throw posix_error{"tcsetattr"};
}

scoped_raw_mode::~scoped_raw_mode()
{
    info() << "Restoring tty attrs";
    tcsetattr(dev_->handle(), TCSANOW, &prev_);
}

////////////////////////////////////////////////////////////////////////////////
scoped_process_switch::scoped_process_switch(std::shared_ptr<device> dev) :
    dev_{std::move(dev)}, sigs_{dev_->executor(), release, acquire}
{
    info() << "Enabling process switch mode";
    command<VT_SETMODE, vt_mode> mode{{
        .mode  = VT_PROCESS,
        .waitv = 0,
        .relsig= release,
        .acqsig= acquire,
        .frsig = 0,
    }};
    dev_->io_control(mode);

    sched_signal_callback();
}

scoped_process_switch::~scoped_process_switch()
{
    info() << "Restoring auto switch mode";
    command<VT_SETMODE, vt_mode> mode{{
        .mode = VT_AUTO,
        .waitv = 0,
        .relsig= 0,
        .acqsig= 0,
        .frsig = 0,
    }};
    dev_->io_control(mode);
}

void scoped_process_switch::sched_signal_callback()
{
    sigs_.async_wait([&](std::error_code ec, int signal)
    {
        if (!ec)
        {
            command<VT_RELDISP, int> rel;
            switch (signal)
            {
            case release:
                info() << "Releasing tty";
                if (release_cb_) release_cb_();

                rel.val = 1;
                dev_->io_control(rel);
                break;

            case acquire:
                info() << "Acquiring tty";
                rel.val = VT_ACKACQ;
                dev_->io_control(rel);

                if (acquire_cb_) acquire_cb_();
                break;
            }

            sched_signal_callback();
        }
    });
}

////////////////////////////////////////////////////////////////////////////////
scoped_graphics_mode::scoped_graphics_mode(std::shared_ptr<device> dev) : dev_{std::move(dev)}
{
    command<KDGETMODE, unsigned*> get_mode{&prev_};
    dev_->io_control(get_mode);

    info() << "Switching to graphics mode";
    command<KDSETMODE, unsigned> set_graph{KD_GRAPHICS};
    dev_->io_control(set_graph);
}

scoped_graphics_mode::~scoped_graphics_mode()
{
    info() << "Restoring previous mode";
    command<KDSETMODE, unsigned> set_mode{prev_};
    dev_->io_control(set_mode);
}

////////////////////////////////////////////////////////////////////////////////
}
