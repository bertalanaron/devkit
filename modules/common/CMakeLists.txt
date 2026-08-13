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
    include/devkit/common/imgui_helpers.h
)

set(COMMON_PRIVATE_SOURCES
    src/common/utils.cpp
)

add_library(devkit_common STATIC
    ${COMMON_PUBLIC_SOURCES}
    ${COMMON_PRIVATE_SOURCES}
)
add_library(devkit::common ALIAS devkit_common)

source_group("common/include" FILES ${COMMON_PUBLIC_SOURCES})
source_group("common/src" FILES ${COMMON_PRIVATE_SOURCES})

target_include_directories(devkit_common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/io/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/gfx/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/algo/include>
    $<BUILD_INTERFACE:${MINI_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(devkit_common PUBLIC
    imgui::imgui
    glm::glm
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    magic_enum::magic_enum
    yaml-cpp::yaml-cpp
    ftxui::dom
    ftxui::screen
    ftxui::component
    cereal::cereal
    Boost::filesystem
)

target_compile_options(devkit_common PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

set_target_properties(devkit_common PROPERTIES
    EXPORT_NAME common
    FOLDER "devkit/modules"
)

if(DEVKIT_BUILD_TESTS)
    add_executable(devkit_common_tests
        tests/common_test.cpp
    )
    target_link_libraries(devkit_common_tests PRIVATE
        devkit::common
        GTest::gtest
        GTest::gtest_main
    )
    set_target_properties(devkit_common_tests PROPERTIES FOLDER "devkit/modules/common/tests")
    gtest_discover_tests(devkit_common_tests)
endif()
