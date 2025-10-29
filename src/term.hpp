////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "drm.hpp"
#include "framebuf.hpp"
#include "pango.hpp"
#include "pty.hpp"
#include "tty.hpp"
#include "vte.hpp"

#include <asio/any_io_executor.hpp>
#include <string>
#include <memory>
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

    using exit_callback = pty::device::child_exit_callback;
    void on_exit(exit_callback cb) { pty_->on_child_exit(std::move(cb)); }

private:
    ////////////////////
    std::unique_ptr<tty::device> tty_;

    std::unique_ptr<drm::device> drm_;
    std::unique_ptr<drm::framebuf> fb_;

    std::unique_ptr<pango::engine> pango_;
    std::unique_ptr<vte::machine> vte_;
    std::unique_ptr<pty::device> pty_;

    unsigned width_, height_, cell_width_, cell_height_;

    ////////////////////
    bool enabled_ = true;
    void enable();
    void disable();

    void change(int row, std::span<const vte::cell>);
    void move(int col, int row, unsigned w, unsigned h, int src_col, int src_row);
};
