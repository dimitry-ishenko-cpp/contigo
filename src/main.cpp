////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "drm.hpp"
#include "logging.hpp"
#include "term.hpp"
#include "tty.hpp"

#include <asio.hpp>
#include <charconv> // std::from_chars
#include <filesystem>
#include <optional>
#include <pgm/args.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

template<typename T>
std::optional<T> get(const pgm::argval&, std::string_view prefix1, std::string_view prefix2, const std::string& name);

void show_usage(const pgm::args&, std::string_view name);
void show_version(std::string_view name);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
try
{
    namespace fs = std::filesystem;
    auto name = fs::path{argv[0]}.filename().string();

    term_options options;

    pgm::args args
    {
        { "-t", "--tty", "ttyN|N",      "Use specified tty; otherwise, use the current one." },
        { "-a", "--activate",           "Activate given tty before starting.\n"},

        { "-g", "--gpu", "cardN|N",     "Use specified graphics adapter; if none given, use the first detected." },
        { "-p", "--dpi", "N",           "Override DPI value reported by the screen." },
        { "-f", "--font", "name",       "Use specified font. Default: '" + options.font + "'\n" },

        { "-s", "--speed", "S",         "Change mouse speed. Default: " + std::to_string(options.mouse_speed) + "\n" },

        { "-v", "--version",            "Print version number and exit" },
        { "-h", "--help",               "Show this help" },

        { "login", pgm::mul | pgm::opt, "Launch specified login program. Default: " + options.login },
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
        auto tty = get<unsigned>(args["--tty"], tty::path, tty::name, "tty path or number");
        options.tty_num = tty.value_or(tty::active(ex));
        options.tty_activate = !!args["--activate"];

        auto gpu = get<unsigned>(args["--gpu"], drm::path, drm::name, "GPU path or number");
        options.drm_num = gpu.value_or(drm::find());

        auto dpi = get<unsigned>(args["--dpi"], {}, {}, "DPI value");
        if (dpi) options.dpi = *dpi;

        auto font = args["--font"];
        if (font) options.font = font.value();

        auto speed = get<float>(args["--speed"], {}, {}, "mouse speed");
        if (speed) options.mouse_speed = *speed;

        options.args = args["login"].values();
        if (options.args.size())
        {
            options.login = std::move(options.args.front());
            options.args.erase(options.args.begin());
        }

        term term{ex, std::move(options)};
        term.on_exited([&](auto){ io.stop(); });

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
template<typename T>
std::optional<T> get(const pgm::argval& argval, std::string_view prefix1, std::string_view prefix2, const std::string& name)
{
    if (argval)
    {
        std::string_view val = argval.value();

        auto off = 0;
        if (val.starts_with(prefix1)) off = prefix1.size();
        else if (val.starts_with(prefix2)) off = prefix2.size();

        T num;
        auto [end, ec] = std::from_chars(val.begin() + off, val.end(), num);

        if (ec != std::errc{} || end != val.end()) throw std::invalid_argument{
            "Invalid " + name + " - " + val.data()
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
