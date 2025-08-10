cmake_minimum_required(VERSION 3.14)
project(CH9329Controller
        VERSION 1.0.0
        DESCRIPTION "C++ library for CH9329 USB-to-Serial device controller"
        LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 选项：是否构建示例
option(BUILD_EXAMPLES "Build example programs" ON)

# 查找依赖库
find_package(Boost REQUIRED COMPONENTS system)

# 创建库（共享和静态）
add_library(CH9329Controller
        src/CH9329Controller.cpp
)

# 目标属性设置
target_include_directories(CH9329Controller
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(CH9329Controller
        PUBLIC
        Boost::system
)

# 设置库版本信息
set_target_properties(CH9329Controller PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
)

# 安装规则
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

# 导出配置文件以便其他项目使用
install(EXPORT CH9329ControllerTargets
        FILE CH9329ControllerConfig.cmake
        NAMESPACE CH9329::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CH9329Controller
)

# 生成 config 文件
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

# 构建示例程序（可选）
if(BUILD_EXAMPLES)
    add_executable(demo examples/demo.cpp)
    target_link_libraries(demo PRIVATE CH9329Controller)
endif()
