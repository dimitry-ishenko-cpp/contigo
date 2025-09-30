////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include <exception>
#include <filesystem>
#include <iostream>
#include <pgm/args.hpp>
#include <string_view>

void show_usage(const pgm::args& args, std::string_view name);
void show_version(std::string_view name);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
try
{
    namespace fs = std::filesystem;
    auto name = fs::path{argv[0]}.filename().string();

    pgm::args args
    {
        { "-v", "--version",    "Print version number and exit" },
        { "-h", "--help",       "Show this help" },
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
        // normal program flow
    }

    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
};

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
