set(IO_PUBLIC_SOURCES
    include/devkit/io.h
    include/devkit/io/asset_manager.h
    include/devkit/io/input_combination.h
    include/devkit/io/window.h
    include/devkit/io/file_dialog.h
    include/devkit/io/frame.h
    include/devkit/io/virtual_filesystem.h
)

set(IO_PRIVATE_SOURCES
    src/io/asset_manager.cpp
    src/io/window.cpp
    src/io/input_combination.cpp
    src/io/file_dialog.cpp
    src/io/frame.cpp
)

add_library(devkit_io STATIC
    ${IO_PUBLIC_SOURCES}
    ${IO_PRIVATE_SOURCES}
)
add_library(devkit::io ALIAS devkit_io)

source_group("io/include" FILES ${IO_PUBLIC_SOURCES})
source_group("io/src" FILES ${IO_PRIVATE_SOURCES})

target_include_directories(devkit_io PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/common/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/gfx/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/algo/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(devkit_io PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/gfx/src>
)

target_link_libraries(devkit_io PUBLIC
    devkit::common
    SDL3::SDL3
    nfd::nfd
    Boost::dll
)

target_compile_options(devkit_io PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

set_target_properties(devkit_io PROPERTIES
    EXPORT_NAME io
    FOLDER "devkit/modules"
)

if(DEVKIT_BUILD_TESTS)
    add_executable(devkit_io_tests
        tests/io_test.cpp
    )
    target_link_libraries(devkit_io_tests PRIVATE
        devkit::io
        GTest::gtest
        GTest::gtest_main
    )
    set_target_properties(devkit_io_tests PROPERTIES FOLDER "devkit/modules/io/tests")
    gtest_discover_tests(devkit_io_tests)
endif()
