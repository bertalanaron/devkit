# Fetch GTest via vcpkg
find_package(GTest CONFIG REQUIRED)

# Add source files
set(COMMON_PUBLIC_SOURCES
    src/common/utils_test.cpp)
set(GFX_PUBLIC_SOURCES
    src/gfx/vertex_buffer_test.cpp)
set(IO_SOURCES
    src/io/asset_manager_test.cpp
    src/io/asset_fetcher_test.cpp
    src/io/input_combination_tests.cpp
    src/io/window_test.cpp)
set(SOURCES ${IO_SOURCES} ${GFX_PUBLIC_SOURCES} ${COMMON_PUBLIC_SOURCES})
source_group("common" FILES ${COMMON_PUBLIC_SOURCES})
source_group("gfx" FILES ${GFX_PUBLIC_SOURCES})
source_group("io" FILES ${IO_SOURCES})

# Create executable for tests
add_executable(utest ${SOURCES})

# Link dependencies
target_link_libraries(utest
    PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main
)

# Register with CTest so Visual Studio's Test Explorer picks it up
include(GoogleTest)
gtest_discover_tests(utest)
