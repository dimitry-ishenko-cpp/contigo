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

using vterm_ptr = std::unique_ptr<VTerm, void(*)(VTerm*)>;
using pixman::color;

////////////////////////////////////////////////////////////////////////////////
struct cell
{
    static constexpr unsigned max_chars = 32;

    char chars[max_chars];
    unsigned width;

    bool bold;
    bool italic;
    bool strike;
    unsigned underline :2;

    color fg, bg;
};

struct cursor
{
    int x, y;
    bool visible;
    bool blink;
    enum { block, hline, vline } shape;
};

////////////////////////////////////////////////////////////////////////////////
class machine
{
public:
    ////////////////////
    explicit machine(unsigned w, unsigned h);

    using row_changed_callback = std::function<void(int y)>;
    void on_row_changed(row_changed_callback cb) { row_cb_ = std::move(cb); }

    using cursor_changed_callback = std::function<void(const cursor&)>;
    void on_cursor_changed(cursor_changed_callback cb) { cursor_cb_ = std::move(cb); }

    using size_changed_callback = std::function<void(unsigned w, unsigned h)>;
    void on_size_changed(size_changed_callback cb) { size_cb_ = std::move(cb); }

    ////////////////////
    void write(std::span<const char>);
    void commit();

    vte::cell cell(int x, int y);
    std::vector<vte::cell> row(int y);

    void resize(unsigned w, unsigned h);

private:
    ////////////////////
    vterm_ptr vterm_;
    VTermScreen* screen_;
    VTermState* state_;

    unsigned width_;
    cursor cursor_;

    row_changed_callback row_cb_;
    cursor_changed_callback cursor_cb_;
    size_changed_callback size_cb_;

    ////////////////////
    struct dispatch;
};

////////////////////////////////////////////////////////////////////////////////
}
