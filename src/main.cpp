////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "tty.hpp"

#include <asio.hpp>
#include <charconv>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <pgm/args.hpp>
#include <string>
#include <string_view>
#include <system_error>

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

        { "login", pgm::mul | pgm::opt,     "Login program (with options) to start. Default: " + def_login },
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

        std::unique_ptr<tty> vt;

        if (auto num = get_vt(args))
        {
            if (args["--activate"])
                vt = std::make_unique<tty>(ex, *num, tty::activate);
            else vt = std::make_unique<tty>(ex, *num);
        }
        else vt = std::make_unique<tty>(ex);

        //
    }

    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
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

    std::cout << args.usage(name, preamble, prologue, epilogue) << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
void show_version(std::string_view name)
{
    std::cout << name << " version " << VERSION << std::endl;
}
