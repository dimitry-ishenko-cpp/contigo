////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdio>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////
class logstream
{
public:
    ////////////////////
    explicit logstream(std::FILE* fs) : fs_{fs} { }

    ~logstream()
    {
        ss_ << std::endl;
        std::fputs(ss_.str().data(), fs_);
        std::fflush(fs_);
    }

    template<typename T>
    auto& operator<<(T&& v)
    {
        ss_ << std::forward<T>(v);
        return *this;
    }

private:
    ////////////////////
    std::ostringstream ss_;
    std::FILE* fs_;
};

inline auto info() { return logstream{stdout}; }
inline auto err() { return logstream{stderr}; }
