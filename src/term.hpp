////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "drm.hpp"
#include "framebuf.hpp"
#include "mouse.hpp"
#include "pango.hpp"
#include "pixman.hpp"
#include "pty.hpp"
#include "tty.hpp"
#include "vte.hpp"

#include <asio/any_io_executor.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
struct term_options
{
    tty::num tty_num;
    bool tty_activate = false;

    drm::num drm_num;
    std::optional<unsigned> dpi;
    std::string font = "monospace, 20";

    std::string login = "/bin/login";
    std::vector<std::string> args;
};

class term
{
public:
    ////////////////////
    term(const asio::any_io_executor&, term_options);

    using exited_callback = pty::device::child_exited_callback;
    void on_exited(exited_callback cb) { pty_->on_child_exited(std::move(cb)); }

private:
    ////////////////////
    std::unique_ptr<tty::device> tty_;

    std::unique_ptr<drm::device> drm_;
    std::unique_ptr<drm::framebuf> fb_;

    std::unique_ptr<pango::engine> pango_;
    std::unique_ptr<vte::machine> vte_;
    std::unique_ptr<pty::device> pty_;

    std::unique_ptr<mouse::device> mouse_;

    drm::mode mode_;
    pango::box box_;
    struct { unsigned rows, cols; } size_;

    bool enabled_ = true;
    void enable();
    void disable();

    void update(int row, int col, unsigned count);
    void commit();

    ////////////////////
    enum kind { mouse, keyboard, size };

    struct cursor
    {
        int row = 0, col = 0;
        vte::cursor state;
    }
    cursor_[kind::size];
    std::optional<pixman::image> patch_[kind::size];

    void move_cursor(kind, int row, int col);
    void change(kind, const vte::cursor&);

    void draw_cursor(kind);
    void undraw_cursor(kind);
};
