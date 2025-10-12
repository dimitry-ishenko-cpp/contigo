////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "pty.hpp"
#include "tty.hpp"

#include <asio.hpp>
#include <charconv>
#include <filesystem>
#include <optional>
#include <pgm/args.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

std::optional<unsigned> get_vt(const pgm::args&);

void show_usage(const pgm::args&, std::string_view name);
void show_version(std::string_view name);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
try
{
    namespace fs = std::filesystem;
    auto name = fs::path{argv[0]}.filename().string();

    const std::string def_login = "/bin/login";

    pgm::args args
    {
        { "-t", "--vt", "/dev/ttyN|ttyN|N", "Virtual terminal to use. If omitted, use the current one." },
        { "-c", "--activate",               "Activate given terminal before starting."},

        { "-v", "--version",                "Print version number and exit" },
        { "-h", "--help",                   "Show this help" },

        { "login", pgm::mul | pgm::opt,     "Login program to launch. Default: " + def_login },
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
        auto num = get_vt(args).value_or(tty::active(ex));
        auto action = args["--activate"] ? tty::activate : tty::dont_activate;
        tty tty{ex, num, action};

        ////////////////////
        std::string login = def_login;

        auto values = args["login"].values();
        if (values.size())
        {
            login = std::move(values.front());
            values.erase(values.begin());
        }

        pty pty{ex, std::move(login), std::move(values)};
        pty.on_child_exit([&](auto){ io.stop(); });

        ////////////////////
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
std::optional<unsigned> get_vt(const pgm::args& args)
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
