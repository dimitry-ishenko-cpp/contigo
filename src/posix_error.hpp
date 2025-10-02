////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#ifndef POSIX_ERROR_HPP
#define POSIX_ERROR_HPP

#include <cerrno>
#include <string>
#include <system_error>

////////////////////////////////////////////////////////////////////////////////
class posix_error : public std::system_error
{
public:
    posix_error() : std::system_error{errno, std::system_category()} { }
    explicit posix_error(const char* msg) : std::system_error{errno, std::system_category(), msg} { }
    explicit posix_error(const std::string& msg) : std::system_error{errno, std::system_category(), msg} { }
};

////////////////////////////////////////////////////////////////////////////////
#endif
