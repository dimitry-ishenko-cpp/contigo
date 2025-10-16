project(screen)

find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)

add_library(screen STATIC
    fb.cpp
    fb.hpp
    screen.cpp
    screen.hpp
)
target_include_directories(screen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(screen PUBLIC PkgConfig::asio)
