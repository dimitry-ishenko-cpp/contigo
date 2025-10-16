project(src)

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

add_subdirectory(screen)

add_executable(contigo
    color.hpp
    command.hpp
    error.hpp
    fb.cpp
    fb.hpp
    logging.hpp
    pty.cpp
    pty.hpp
    term.cpp
    term.hpp
    tty.cpp
    tty.hpp
    types.hpp
    vte.cpp
    vte.hpp
    main.cpp
)
target_link_libraries(contigo PRIVATE pgm::args PkgConfig::asio PkgConfig::vterm screen)

install(TARGETS contigo DESTINATION ${CMAKE_INSTALL_BINDIR})
