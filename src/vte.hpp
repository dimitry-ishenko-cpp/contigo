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
    int row, col;
    bool visible;
    bool blink;
    enum { block, hline, vline } shape;
};

////////////////////////////////////////////////////////////////////////////////
class machine
{
public:
    ////////////////////
    explicit machine(unsigned rows, unsigned cols);

    using output_data_callback = std::function<void(std::span<const char>)>;
    void on_output_data(output_data_callback cb) { output_cb_ = std::move(cb); }

    using row_changed_callback = std::function<void(int row)>;
    void on_row_changed(row_changed_callback cb) { row_cb_ = std::move(cb); }

    using cursor_changed_callback = std::function<void(const cursor&)>;
    void on_cursor_changed(cursor_changed_callback cb) { cursor_cb_ = std::move(cb); }

    using size_changed_callback = std::function<void(unsigned rows, unsigned cols)>;
    void on_size_changed(size_changed_callback cb) { size_cb_ = std::move(cb); }

    ////////////////////
    void write(std::span<const char>);
    void commit();

    vte::cell cell(int row, int col);
    std::vector<vte::cell> cells(int row);

    void resize(unsigned rows, unsigned cols);

private:
    ////////////////////
    vterm_ptr vterm_;
    VTermScreen* screen_;
    VTermState* state_;

    unsigned cols_;
    cursor cursor_;

    output_data_callback output_cb_;
    row_changed_callback row_cb_;
    cursor_changed_callback cursor_cb_;
    size_changed_callback size_cb_;

    ////////////////////
    struct dispatch;
};

////////////////////////////////////////////////////////////////////////////////
}
