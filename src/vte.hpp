////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "pixman.hpp"

#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <vterm.h>

////////////////////////////////////////////////////////////////////////////////
namespace vte
{

using attrs = VTermScreenCellAttrs;
using vterm_ptr = std::unique_ptr<VTerm, void(*)(VTerm*)>;

////////////////////////////////////////////////////////////////////////////////
struct cell
{
    static constexpr unsigned max_chars = 32;

    char chars[max_chars];
    std::size_t len;
    unsigned width;
    vte::attrs attrs;
    pixman::color fg, bg;
};

struct cursor
{
    bool visible;
    bool blink;
    enum shape { block, hline, vline } shape;
};

enum button : unsigned { button_left = 1, button_mid, button_right };
enum wheel  : unsigned { wheel_up = 4, wheel_down, wheel_left, wheel_right };

////////////////////////////////////////////////////////////////////////////////
class machine
{
public:
    ////////////////////
    machine(unsigned rows, unsigned cols);

    using send_data_callback = std::function<void(std::span<const char>)>;
    void on_send_data(send_data_callback cb) { send_cb_ = std::move(cb); }

    using row_changed_callback = std::function<void(int row, int col, unsigned count)>;
    void on_row_changed(row_changed_callback cb) { row_cb_ = std::move(cb); }

    using cursor_moved_callback = std::function<void(int row, int col)>;
    void on_cursor_moved(cursor_moved_callback cb) { move_cb_ = std::move(cb); }

    using cursor_changed_callback = std::function<void(const cursor&)>;
    void on_cursor_changed(cursor_changed_callback cb) { cursor_cb_ = std::move(cb); }

    using size_changed_callback = std::function<void(unsigned rows, unsigned cols)>;
    void on_size_changed(size_changed_callback cb) { size_cb_ = std::move(cb); }

    ////////////////////
    void recv(std::span<const char>);
    void send(std::span<const char>);
    void commit();

    std::vector<vte::cell> cells(int row, int col, unsigned count);

    void resize(unsigned rows, unsigned cols);

    void move_mouse(int row, int col);
    void change(vte::button, bool state);

  private:
    ////////////////////
    vterm_ptr vterm_;
    VTermScreen* screen_;
    VTermState* state_;

    send_data_callback send_cb_;
    row_changed_callback row_cb_;

    cursor_moved_callback move_cb_;
    cursor cursor_;
    cursor_changed_callback cursor_cb_;

    size_changed_callback size_cb_;

    vte::cell cell(int row, int col);

    ////////////////////
    struct dispatch;
};

////////////////////////////////////////////////////////////////////////////////
}
