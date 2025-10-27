##

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_search_module(pangoft2 REQUIRED IMPORTED_TARGET pangoft2)
pkg_search_module(pixman-1 REQUIRED IMPORTED_TARGET pixman-1)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

add_executable(term
    cell.hpp
    command.hpp
    drm.cpp
    drm.hpp
    error.hpp
    framebuf.cpp
    framebuf.hpp
    logging.hpp
    main.cpp
    pango.cpp
    pango.hpp
    pixman.hpp
    pty.cpp
    pty.hpp
    term.cpp
    term.hpp
    tty.cpp
    tty.hpp
    vte.cpp
    vte.hpp
)
target_link_libraries(term PRIVATE pgm::args
    PkgConfig::asio
    PkgConfig::drm
    PkgConfig::pangoft2
    PkgConfig::pixman-1
    PkgConfig::vterm
)

install(TARGETS term DESTINATION ${CMAKE_INSTALL_BINDIR})
