cmake_minimum_required(VERSION 3.21)

# Use vcpkg by default
if (EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  set(VCPKG_TARGET_TRIPLET "x64-windows"
      CACHE STRING "default vcpkg triplet")
endif ()

project(devkit)

# Enable C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(/std:c++latest)
add_compile_options(/bigobj)
add_compile_options(/Zc:preprocessor)

# Find packages
find_package(GLEW CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")

set(INCLUDE_DIR include/${PROJECT_NAME})

# Add source files
set(SOURCES 
    src/devkit.cpp 
    src/graphics_includes.h
    ${INCLUDE_DIR}/devkit.h
    ${INCLUDE_DIR}/log.h)
    
link_directories(${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${CMAKE_VCPKG_TARGET_TRIPLET}/lib)
link_directories(${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${CMAKE_VCPKG_TARGET_TRIPLET}/lib/manual-link)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2 SDL2::SDL2main GLEW::GLEW imgui::imgui)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})

add_executable(examples_basic examples/basic/examples_basic.cpp)
target_link_libraries(examples_basic PRIVATE ${PROJECT_NAME})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT examples_basic)
