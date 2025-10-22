##

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_search_module(pangoft2 REQUIRED IMPORTED_TARGET pangoft2)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

add_executable(term
    cell.hpp
    color.hpp
    command.hpp
    drm.cpp
    drm.hpp
    error.hpp
    geom.hpp
    image.hpp
    logging.hpp
    main.cpp
    pango.cpp
    pango.hpp
    pty.cpp
    pty.hpp
    term.cpp
    term.hpp
    tty.cpp
    tty.hpp
    vte.cpp
    vte.hpp
)
target_link_libraries(term PRIVATE pgm::args PkgConfig::asio PkgConfig::drm PkgConfig::pangoft2 PkgConfig::vterm)

install(TARGETS term DESTINATION ${CMAKE_INSTALL_BINDIR})
