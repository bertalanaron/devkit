cmake_minimum_required(VERSION 3.21)

project(devkit LANGUAGES CXX)

# Options
option(DEVKIT_BUILD_TESTS "Build devkit tests" OFF)
option(DEVKIT_BUILD_EXAMPLES "Build devkit examples" OFF)

if(DEVKIT_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")

# Set Compile Options
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
        # MSVC-only flags
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>

        # GCC (and Clang) equivalent flags
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-pedantic>
)

# List source files
# common
set(COMMON_PUBLIC_SOURCES
    include/devkit/common.h
    include/devkit/common/utils.h
    include/devkit/common/properties.h
    include/devkit/common/constants.h
    include/devkit/common/move_only_any.h
    include/devkit/common/runtime_buffer.h
    include/devkit/common/performance_monitor.h
    include/devkit/common/filesystem_hierarchy_node.h
    include/devkit/common/typeless.h
    include/devkit/common/imgui_helpers.h)
set(COMMON_PRIVATE_SOURCES
    src/common/utils.cpp)
# io
set(IO_PUBLIC_SOURCES
    include/devkit/io.h
    include/devkit/io/asset_manager.h
    include/devkit/io/input_combination.h
    include/devkit/io/window.h
    include/devkit/io/file_dialog.h
    include/devkit/io/frame.h
    include/devkit/io/virtual_filesystem.h)
set(IO_PRIVATE_SOURCES
    src/io/asset_manager.cpp
    src/io/window.cpp
    src/io/input_combination.cpp
    src/io/file_dialog.cpp
    src/io/frame.cpp)
# gfx
set(GFX_PUBLIC_SOURCES
    include/devkit/gfx.h
    include/devkit/gfx/mesh.h
    include/devkit/gfx/common.h
    include/devkit/gfx/scene.h
    include/devkit/gfx/draw_data.h
    include/devkit/gfx/shader.h
    include/devkit/gfx/vertex.h
    include/devkit/gfx/uniforms.h
    include/devkit/gfx/frame_buffer.h
    include/devkit/gfx/vertex_buffer.h
    include/devkit/gfx/camera.h
    include/devkit/gfx/texture.h
    include/devkit/gfx/element_buffer.h
    include/devkit/gfx/font.h
    include/devkit/gfx/vertex_sink.h
    include/devkit/gfx/attachment_base.h
    include/devkit/gfx/viewport.h
    include/devkit/gfx/api_resources.h)
set(GFX_PRIVATE_SOURCES
    src/gfx/common.cpp
    src/gfx/vertex.cpp
    src/gfx/uniforms.cpp
    src/gfx/scene.cpp
    src/gfx/model.cpp
    src/gfx/shader.cpp
    src/gfx/frame_buffer.cpp
    src/gfx/vertex_buffer.cpp
    src/gfx/camera.cpp
    src/gfx/element_buffer.cpp
    src/gfx/mesh.cpp
    src/gfx/texture.cpp
    src/gfx/font.cpp
    src/gfx/vertex_sink.cpp
    src/gfx/attachment_base.cpp
    src/gfx/viewport.cpp
    src/gfx/context.h
    src/gfx/context.cpp
    src/gfx/api_resources.cpp
    src/gfx/demo_scene.h)
# algo
set(ALGO_PUBLIC_SOURCES
    include/devkit/algo.h
    include/devkit/algo/geometry.h
    include/devkit/algo/draw.h
    include/devkit/algo/navmesh.h
    include/devkit/algo/navmesh_pathfinding.h
    include/devkit/algo/task_queue.h
    include/devkit/algo/funnel.h
    include/devkit/algo/chunk_accessor.h
    include/devkit/algo/flood_fill.h
    include/devkit/algo/marching_squares.h)
set(ALGO_PRIVATE_SOURCES
    src/algo/geometry.cpp
    src/algo/navmesh.cpp
    src/algo/navmesh_pathfinding.cpp
    src/algo/funnel.cpp
    src/algo/polypartition/polypartition.h
    src/algo/polypartition/polypartition.cpp)
set(SOURCES 
    ${COMMON_PUBLIC_SOURCES}
    ${COMMON_PRIVATE_SOURCES}
    ${IO_PUBLIC_SOURCES} 
    ${IO_PRIVATE_SOURCES}
    ${GFX_PUBLIC_SOURCES}
    ${GFX_PRIVATE_SOURCES}
    ${ALGO_PUBLIC_SOURCES}
    ${ALGO_PRIVATE_SOURCES}
    )
source_group("common/include" FILES ${COMMON_PUBLIC_SOURCES})
source_group("common/src" FILES ${COMMON_PRIVATE_SOURCES})
source_group("io/include" FILES ${IO_PUBLIC_SOURCES})
source_group("io/src" FILES ${IO_PRIVATE_SOURCES})
source_group("gfx/include" FILES ${GFX_PUBLIC_SOURCES})
source_group("gfx/src" FILES ${GFX_PRIVATE_SOURCES})
source_group("algo/include" FILES ${ALGO_PUBLIC_SOURCES})
source_group("algo/src" FILES ${ALGO_PRIVATE_SOURCES})

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(devkit::devkit ALIAS devkit)

# Include library headers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# Find dependencies
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
find_package(Boost REQUIRED COMPONENTS dll)
find_path(MINI_INCLUDE_DIRS "mini/ini.h")
find_package(poly2tri REQUIRED)
find_package(cereal CONFIG REQUIRED)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 glad::glad assimp::assimp nfd::nfd Boost::dll poly2tri::poly2tri)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui glm::glm nlohmann_json::nlohmann_json spdlog::spdlog_header_only magic_enum::magic_enum yaml-cpp::yaml-cpp ftxui::dom ftxui::screen ftxui::component cereal::cereal)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${MINI_INCLUDE_DIRS}>)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})

# Set compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

# Install the library binary
install(TARGETS devkit 
    EXPORT devkit-targets
    DESTINATION lib
)

# Install the headers
install(DIRECTORY include/ DESTINATION include)

# Export the targets to a file so find_package works
install(EXPORT devkit-targets
    FILE devkit-targets.cmake
    NAMESPACE devkit::
    DESTINATION share/devkit
)

# Generate and install proper config file with find_dependency calls
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/devkitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/devkitConfig.cmake"
    INSTALL_DESTINATION share/devkit
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/devkitConfig.cmake"
    DESTINATION share/devkit
)

# Add tests
if(DEVKIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

