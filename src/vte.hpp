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
using vterm = std::unique_ptr<VTerm, void(*)(VTerm*)>;

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

    using size_changed_callback = std::function<void(dim)>;
    void on_size_changed(size_changed_callback cb) { size_cb_ = std::move(cb); }

    ////////////////////
    void write(std::span<const char>);
    void commit();

    void resize(dim);
    void reload();

private:
    ////////////////////
    vterm vterm_;
    VTermScreen* screen_;
    VTermState* state_;
    dim dim_;

    row_changed_callback row_cb_;
    rows_moved_callback move_cb_;
    size_changed_callback size_cb_;

    void change(int row);

    ////////////////////
    struct dispatch;
};
