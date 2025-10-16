project(render)

find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(fontconfig REQUIRED IMPORTED_TARGET fontconfig)
pkg_search_module(freetype2 REQUIRED IMPORTED_TARGET freetype2)
pkg_search_module(pangoft2 REQUIRED IMPORTED_TARGET pangoft2)

add_library(render STATIC
    fontconfig.cpp
    fontconfig.hpp
    freetype.cpp
    freetype.hpp
    pango.cpp
    pango.hpp
)
target_include_directories(render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(render PUBLIC core PkgConfig::asio PkgConfig::fontconfig PkgConfig::freetype2 PkgConfig::pangoft2)
