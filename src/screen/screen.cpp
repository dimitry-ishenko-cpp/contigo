////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "screen.hpp"

screen::screen(const asio::any_io_executor& ex, fb::num num, screen_options options) :
    fb_{ex, num}, dpi_{options.dpi.value_or(fb_.dpi())}
{
    fb_.on_frame_sync([&]
    {
        if (enabled_) fb_.present();
    });
}

void screen::enable(bool val)
{
    if (val != enabled_)
    {
        info() << (val ? "Enabling" : "Disabling") << " screen rendering";
        enabled_ = val;
    }
}
