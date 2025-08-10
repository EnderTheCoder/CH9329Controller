cmake_minimum_required(VERSION 3.14)
project(CH9329Controller
        VERSION 1.0.0
        DESCRIPTION "C++ library for CH9329 USB-to-Serial device controller"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_EXAMPLES "Build example programs" ON)

find_package(Boost REQUIRED COMPONENTS system)

add_library(CH9329Controller
        src/CH9329Controller.cpp
)

target_include_directories(CH9329Controller
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(CH9329Controller
        PUBLIC
        Boost::system
)

set_target_properties(CH9329Controller PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
)

include(GNUInstallDirs)
install(TARGETS CH9329Controller
        EXPORT CH9329ControllerTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT CH9329ControllerTargets
        FILE CH9329ControllerConfig.cmake
        NAMESPACE CH9329::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CH9329Controller
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/CH9329ControllerConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CH9329ControllerConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CH9329Controller
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CH9329ControllerConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CH9329ControllerConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CH9329ControllerConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CH9329Controller
)

if(BUILD_EXAMPLES)
    add_executable(demo examples/demo.cpp)
    target_link_libraries(demo PRIVATE CH9329Controller)
endif()
