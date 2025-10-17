##

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

add_executable(term
    cell.hpp
    color.hpp
    command.hpp
    error.hpp
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
target_link_libraries(term PRIVATE pgm::args PkgConfig::asio PkgConfig::vterm)

install(TARGETS term DESTINATION ${CMAKE_INSTALL_BINDIR})
