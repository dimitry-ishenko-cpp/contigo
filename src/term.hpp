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

    using exit_callback = pty::child_exit_callback;
    void on_exit(exit_callback cb) { pty_.on_child_exit(std::move(cb)); }

private:
    ////////////////////
    std::shared_ptr<tty::device> tty_;
    tty::scoped_active tty_active_;
    tty::scoped_raw_mode tty_raw_;
    tty::scoped_process_switch tty_switch_;
    tty::scoped_graphics_mode tty_graph_;

    std::shared_ptr<drm::device> drm_;
    drm::crtc drm_crtc_;
    drm::framebuf<color> drm_fb_;

    pango::engine engine_;
    vte vte_;
    pty pty_;

    ////////////////////
    bool enabled_ = true;
    void enable();
    void disable();

    void change(int row, std::span<const cell>);
    void move(int row, unsigned rows, int distance);
};
