////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "term.hpp"

term::term(const asio::any_io_executor& ex, term_options options)
{
    tty_ = std::make_unique<tty>(ex, options.tty_num, options.tty_activate);
    tty_->on_acquire([&]{ enable(); });
    tty_->on_release([&]{ disable(); });

    fb_  = std::make_unique<fb >(ex, options.fb_num);
    fb_->on_frame_sync([&]{ draw(); });
    dpi_ = options.dpi.value_or(fb_->dpi());

    vte_ = std::make_unique<vte>(size{80, 24});

    pty_ = std::make_unique<pty>(ex, std::move(options.login), std::move(options.args));

    tty_->on_read_data([&](auto data){ pty_->write(data); });
    pty_->on_read_data([&](auto data){ vte_->write(data); });
}

void term::enable()
{
    info() << "Enabling screen rendering";
    enabled_ = true;

    vte_->redraw();
}

void term::disable()
{
    info() << "Disabling screen rendering";
    enabled_ = false;
}

void term::draw()
{
    if (enabled_)
    {
        fb_->update();

        // TODO: copy shadow to fb
    }

    vte_->commit();
}
