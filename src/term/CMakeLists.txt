project(term)

find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

add_library(term STATIC
    pty.cpp
    pty.hpp
    tty.cpp
    tty.hpp
    vte.cpp
    vte.hpp
)
target_include_directories(term PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(term PUBLIC core PkgConfig::asio PkgConfig::vterm)
