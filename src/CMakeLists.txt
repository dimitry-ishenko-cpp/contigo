project(src)

find_package(pgm_args REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(asio REQUIRED IMPORTED_TARGET asio)

add_library(core INTERFACE
    color.hpp
    error.hpp
    io.hpp
    logging.hpp
)
target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(screen)
add_subdirectory(term)

add_executable(contigo
    main.cpp
)
target_link_libraries(contigo PRIVATE core pgm::args PkgConfig::asio screen term)

install(TARGETS contigo DESTINATION ${CMAKE_INSTALL_BINDIR})
