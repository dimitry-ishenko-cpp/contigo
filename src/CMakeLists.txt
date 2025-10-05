# src

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_NO_DEPRECATED)
add_definitions(-DVERSION="${PROJECT_VERSION}")

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)
pkg_search_module(vterm REQUIRED IMPORTED_TARGET vterm)

set(name contigo)
add_executable(${name}
    error.hpp
    logging.hpp
    main.cpp
    tty.cpp
    tty.hpp
    vterm.cpp
    vterm.hpp
)
target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${name} PRIVATE pgm::args PkgConfig::asio PkgConfig::vterm)

install(TARGETS ${name} DESTINATION ${CMAKE_INSTALL_BINDIR})
