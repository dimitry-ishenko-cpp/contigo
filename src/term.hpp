////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

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
    bool activate = false;

    std::string login = "/bin/login";
    std::vector<std::string> args;
};

class term
{
public:
    ////////////////////
    term(const asio::any_io_executor&, tty::num, term_options = {});

    using finished_callback = pty::child_exit_callback;
    void on_finished(finished_callback cb) { pty_->on_child_exit(std::move(cb)); }

    void on_acquire(tty::acquire_callback cb) { tty_->on_acquire(std::move(cb)); }
    void on_release(tty::release_callback cb) { tty_->on_release(std::move(cb)); }

    void on_row_changed(vte::row_changed_callback cb) { vte_->on_row_changed(std::move(cb)); }
    void on_rows_moved(vte::rows_moved_callback cb) { vte_->on_rows_moved(std::move(cb)); }

    void resize(const size& size) { vte_->resize(size); }

    ////////////////////
    static tty::num active(const asio::any_io_executor& ex) { return tty::active(ex); }

private:
    ////////////////////
    std::unique_ptr<tty> tty_;
    std::unique_ptr<pty> pty_;
    std::unique_ptr<vte> vte_;
};
