cmake_minimum_required(VERSION 3.21)

project(devkit LANGUAGES CXX)

option(DEVKIT_BUILD_TESTS "Build devkit tests" OFF)

if(DEVKIT_BUILD_TESTS)
    include(CTest)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-pedantic>
)

find_package(glad CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(assimp REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS dll filesystem bimap)
find_path(MINI_INCLUDE_DIRS "mini/ini.h")
find_package(poly2tri REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)

add_subdirectory(modules/common)
add_subdirectory(modules/io)
add_subdirectory(modules/algo)
add_subdirectory(modules/gfx)

target_link_libraries(devkit_io PUBLIC devkit::gfx)
target_link_libraries(devkit_algo PUBLIC devkit::gfx)
target_link_libraries(devkit_gfx PUBLIC devkit::io devkit::algo)

add_library(devkit INTERFACE
        "../examples/01 basic/src/primitives.h"
        "../examples/01 basic/src/render_context.h"
        "../examples/01 basic/src/render_context.cpp")
add_library(devkit::devkit ALIAS devkit)
target_link_libraries(devkit INTERFACE
    devkit::common
    devkit::io
    devkit::algo
    devkit::gfx
)

install(TARGETS devkit devkit_common devkit_io devkit_algo devkit_gfx
    EXPORT devkit-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY
    modules/common/include/
    modules/io/include/
    modules/algo/include/
    modules/gfx/include/
    DESTINATION include
)

install(EXPORT devkit-targets
    FILE devkit-targets.cmake
    NAMESPACE devkit::
    DESTINATION share/devkit
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/devkitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/devkitConfig.cmake"
    INSTALL_DESTINATION share/devkit
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/devkitConfig.cmake"
    DESTINATION share/devkit
)
