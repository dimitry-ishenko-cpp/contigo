////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "fb.hpp"
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

    fb::num fb_num = 0;
    std::optional<unsigned> dpi;

    std::string login = "/bin/login";
    std::vector<std::string> args;
};

class term
{
public:
    ////////////////////
    term(const asio::any_io_executor&, term_options);

    using finished_callback = pty::child_exit_callback;
    void on_finished(finished_callback cb) { pty_->on_child_exit(std::move(cb)); }

    void resize(const size& size) { vte_->resize(size); }

    ////////////////////
    static tty::num active(const asio::any_io_executor& ex) { return tty::active(ex); }

private:
    ////////////////////
    std::unique_ptr<tty> tty_;
    std::unique_ptr<fb >  fb_;
    std::unique_ptr<vte> vte_;
    std::unique_ptr<pty> pty_;

    ////////////////////
    bool enabled_ = true;
    void enable();
    void disable();

    ////////////////////
    unsigned dpi_;

    void draw();
};
