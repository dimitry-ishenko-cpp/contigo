////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "mouse.hpp"
#include "vte.hpp"

#include <algorithm> // std::clamp
#include <asio/buffer.hpp>
#include <asio/read.hpp>

#include <fcntl.h> // open

////////////////////////////////////////////////////////////////////////////////
namespace mouse
{

namespace
{

auto open(const asio::any_io_executor& ex)
{
    auto fd = ::open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (fd < 0) throw posix_error{"open"};

    return asio::posix::stream_descriptor{ex, fd};
}

}

////////////////////////////////////////////////////////////////////////////////
device::device(const asio::any_io_executor& ex, unsigned rows, unsigned cols, float speed) :
    fd_{open(ex)}, speed_{speed}
{
    resize(rows, cols);
    sched_async_read();
}

void device::sched_async_read()
{
    asio::async_read(fd_, asio::buffer(&event_, sizeof(event_)), [&](std::error_code ec, std::size_t)
    {
        if (!ec)
        {
            row_ = std::clamp(row_ - event_.dy * speed_, 0.f, max_row_);
            col_ = std::clamp(col_ + event_.dx * speed_, 0.f, max_col_);
            if (active_ && move_cb_) move_cb_(row_, col_);

            auto& state = event_.state;
            if (active_ && button_cb_)
            {
                if (state.left  != state_.left ) { button_cb_(vte::button_left , state.left ); }
                if (state.mid   != state_.mid  ) { button_cb_(vte::button_mid  , state.mid  ); }
                if (state.right != state_.right) { button_cb_(vte::button_right, state.right); }
            }
            state_ = state;

            sched_async_read();
        }
    });
}

void device::resize(unsigned rows, unsigned cols)
{
    max_row_ = rows - 1;
    max_col_ = cols - 1;
}

////////////////////////////////////////////////////////////////////////////////
}
