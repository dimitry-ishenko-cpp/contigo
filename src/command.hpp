////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include <type_traits>

template<unsigned Op, typename T>
struct command
{
    T val;
    constexpr auto data() noexcept
    {
        if constexpr (std::is_fundamental_v<T>) return reinterpret_cast<void*>(val);
        else if constexpr (std::is_pointer_v<T>) return static_cast<void*>(val);
        else return static_cast<void*>(&val);
    }
    constexpr auto name() const noexcept { return Op; }
};
