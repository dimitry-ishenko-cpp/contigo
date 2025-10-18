////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cell.hpp"
#include "geom.hpp"

#include <functional>
#include <memory>
#include <span>

struct VTerm;
struct VTermScreen;
struct VTermState;

////////////////////////////////////////////////////////////////////////////////
class vte
{
public:
    ////////////////////
    explicit vte(dim);
    ~vte();

    using row_changed_callback = std::function<void(int, std::span<const cell>)>;
    void on_row_changed(row_changed_callback cb) { row_cb_ = std::move(cb); }

    using rows_moved_callback = std::function<void(int row, unsigned rows, int distance)>;
    void on_rows_moved(rows_moved_callback cb) { move_cb_ = std::move(cb); }

    ////////////////////
    void write(std::span<const char>);
    void commit();

    void redraw();

private:
    ////////////////////
    std::unique_ptr<VTerm, void(*)(VTerm*)> vterm_;
    VTermScreen* screen_;
    VTermState* state_;

    row_changed_callback row_cb_;
    rows_moved_callback move_cb_;

    void change_row(int row, unsigned cols);

    ////////////////////
    struct dispatch;
};
