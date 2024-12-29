cmake_minimum_required(VERSION 3.21)

# Use vcpkg by default
if (EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  set(VCPKG_TARGET_TRIPLET "x64-windows"
      CACHE STRING "default vcpkg triplet")
endif ()

project(devkit)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")

# Set compile options
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(/std:c++latest)
add_compile_options(/bigobj)
add_compile_options(/Zc:preprocessor)

# Add source files
set(INCLUDE_DIR include/${PROJECT_NAME})
set(PUBLIC_SOURCES
    ${INCLUDE_DIR}/devkit.h
    ${INCLUDE_DIR}/log.h
    ${INCLUDE_DIR}/util.h
    ${INCLUDE_DIR}/graphics.h
    ${INCLUDE_DIR}/asset_manager.h)
set(PRIVATE_SOURCES 
    src/devkit.cpp 
    src/graphics.cpp 
    src/graphics_includes.h
    src/gl_types.cpp)
set(SOURCES ${PRIVATE_SOURCES} ${PUBLIC_SOURCES})
source_group("include" FILES ${PUBLIC_SOURCES})
source_group("src" FILES ${PRIVATE_SOURCES})

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(devkit::devkit ALIAS devkit)

# Include library headers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Find dependencies
find_package(GLEW CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main GLEW::GLEW)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui glm::glm nlohmann_json::nlohmann_json spdlog::spdlog_header_only)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
