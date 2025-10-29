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

////////////////////////////////////////////////////////////////////////////////
class machine
{
public:
    ////////////////////
    explicit machine(unsigned w, unsigned h);

    using row_changed_callback = std::function<void(int, std::span<const cell>)>;
    void on_row_changed(row_changed_callback cb) { row_cb_ = std::move(cb); }

    using cells_moved_callback = std::function<void(int col, int row, unsigned w, unsigned h, int src_col, int src_row)>;
    void on_cells_moved(cells_moved_callback cb) { move_cb_ = std::move(cb); }

    using size_changed_callback = std::function<void(unsigned w, unsigned h)>;
    void on_size_changed(size_changed_callback cb) { size_cb_ = std::move(cb); }

    ////////////////////
    void write(std::span<const char>);
    void commit();

    constexpr auto width() const noexcept { return width_; }
    constexpr auto height() const noexcept { return height_; }

    void resize(unsigned w, unsigned h);

private:
    ////////////////////
    vterm_ptr vterm_;
    VTermScreen* screen_;
    VTermState* state_;

    unsigned width_, height_;

    row_changed_callback row_cb_;
    cells_moved_callback move_cb_;
    size_changed_callback size_cb_;

    void change(int row);

    ////////////////////
    struct dispatch;
};

////////////////////////////////////////////////////////////////////////////////
}
