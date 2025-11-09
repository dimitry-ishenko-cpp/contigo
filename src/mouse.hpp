////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <cstdint>
#include <functional>

namespace vte { enum button : unsigned; }

////////////////////////////////////////////////////////////////////////////////
namespace mouse
{

////////////////////////////////////////////////////////////////////////////////
class device
{
public:
    ////////////////////
    device(const asio::any_io_executor&, unsigned rows, unsigned cols, float speed = 1);

    using moved_callback = std::function<void(int row, int col)>;
    void on_moved(moved_callback cb) { move_cb_ = std::move(cb); }

    using button_changed_callback = std::function<void(vte::button, bool state)>;
    void on_button_changed(button_changed_callback cb) { button_cb_ = std::move(cb); }

    void resize(unsigned rows, unsigned cols);

  private:
    ////////////////////
    asio::posix::stream_descriptor fd_;

    moved_callback move_cb_;
    button_changed_callback button_cb_;

    float max_row_, max_col_;
    float speed_;
    float row_ = 0, col_ = 0;

#pragma pack(push, 1)
    struct button
    {
        bool left :1, right :1, mid :1;
    }
    state_;
    static_assert(sizeof(button) == 1);

    struct event
    {
        button state;
        std::int8_t dx, dy;
    }
    event_;
    static_assert(sizeof(event) == 3);
#pragma pack(pop)

    void sched_async_read();
};

////////////////////////////////////////////////////////////////////////////////
}
