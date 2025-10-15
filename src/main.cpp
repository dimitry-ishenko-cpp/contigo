////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "screen.hpp"
#include "term.hpp"

#include <asio.hpp>
#include <charconv>
#include <filesystem>
#include <optional>
#include <pgm/args.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

std::optional<tty::num> get_vt(const pgm::args&);
std::optional<fb::num> get_fb(const pgm::args&);

void show_usage(const pgm::args&, std::string_view name);
void show_version(std::string_view name);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
try
{
    namespace fs = std::filesystem;
    auto name = fs::path{argv[0]}.filename().string();

    term_options term_options;

    constexpr fb::num def_fb = 0;
    screen_options screen_options;

    pgm::args args
    {
        { "-t", "--vt", "/dev/ttyN|ttyN|N", "Virtual terminal to use. If omitted, use the current one." },
        { "-c", "--activate",               "Activate given terminal before starting."},

        { "-b", "--fb", "/dev/fbN|fbN|N",   "Framebuffer to use. Default: fb" + std::to_string(def_fb) },
        { "-p", "--dpi", "N",               "Override DPI value reported by the screen." },

        { "-v", "--version",                "Print version number and exit" },
        { "-h", "--help",                   "Show this help" },

        { "login", pgm::mul | pgm::opt,     "Login program to launch. Default: " + term_options.login },
    };

    std::exception_ptr ep;
    try { args.parse(argc, argv); }
    catch (...) { ep = std::current_exception(); }

    if (args["--help"])
        show_usage(args, name);

    else if (args["--version"])
        show_version(name);

    else if (ep)
        std::rethrow_exception(ep);

    else
    {
        asio::io_context io;
        auto ex = io.get_executor();

        asio::signal_set signals{ex, SIGINT, SIGTERM};
        signals.async_wait([&](std::error_code ec, int signal)
        {
            if (!ec)
            {
                info() << "Received signal " << signal << " - exiting";
                io.stop();
            }
        });

        ////////////////////
        auto tty = get_vt(args).value_or(term::active(ex));
        term_options.activate = !!args["--activate"];

        term_options.args = args["login"].values();
        if (term_options.args.size())
        {
            term_options.login = std::move(term_options.args.front());
            term_options.args.erase(term_options.args.begin());
        }

        term term{ex, tty, std::move(term_options)};
        term.on_finished([&](auto){ io.stop(); });

        ////////////////////
        auto fb = get_fb(args).value_or(def_fb);
        screen screen{ex, fb};

        ////////////////////
        term.on_release([&]{ screen.disable(); });
        term.on_acquire([&]{ screen.enable(); });
        screen.enable();

        io.run();
    }

    return 0;
}
catch (const std::exception& e)
{
    err() << e.what();
    return 1;
};

////////////////////////////////////////////////////////////////////////////////
std::optional<tty::num> get_vt(const pgm::args& args)
{
    if (auto vt = args["--vt"])
    {
        std::string_view val = vt.value();
        auto off = val.starts_with("/dev/tty") ? 8 : val.starts_with("tty") ? 3 : 0;

        unsigned num;
        auto [end, ec] = std::from_chars(val.begin() + off, val.end(), num);

        if (ec != std::errc{} || end != val.end()) throw std::invalid_argument{
            "Invalid terminal - " + args["--vt"].value()
        };
        return num;
    }
    else return std::nullopt;
}

std::optional<fb::num> get_fb(const pgm::args& args)
{
    if (auto vt = args["--fb"])
    {
        std::string_view val = vt.value();
        auto off = val.starts_with("/dev/fb") ? 7 : val.starts_with("fb") ? 2 : 0;

        unsigned num;
        auto [end, ec] = std::from_chars(val.begin() + off, val.end(), num);

        if (ec != std::errc{} || end != val.end()) throw std::invalid_argument{
            "Invalid framebuffer - " + args["--fb"].value()
        };
        return num;
    }
    else return std::nullopt;
}

////////////////////////////////////////////////////////////////////////////////
void show_usage(const pgm::args& args, std::string_view name)
{
    auto preamble =
R"()";

    auto prologue =
R"()";

    auto epilogue =
R"()";

    info() << args.usage(name, preamble, prologue, epilogue);
}

////////////////////////////////////////////////////////////////////////////////
void show_version(std::string_view name)
{
    info() << name << " version " << VERSION;
}
