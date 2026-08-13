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
    include/devkit/gfx/api_resources.h
)

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
    src/gfx/demo_scene.h
)

add_library(devkit_gfx STATIC
    ${GFX_PUBLIC_SOURCES}
    ${GFX_PRIVATE_SOURCES}
)
add_library(devkit::gfx ALIAS devkit_gfx)

source_group("gfx/include" FILES ${GFX_PUBLIC_SOURCES})
source_group("gfx/src" FILES ${GFX_PRIVATE_SOURCES})

target_include_directories(devkit_gfx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/common/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/io/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/algo/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(devkit_gfx PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    ${SDL3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(devkit_gfx PUBLIC
    devkit::common
    glad::glad
    SDL3::SDL3
    imgui::imgui
    glm::glm
    assimp::assimp
)

target_compile_options(devkit_gfx PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

set_target_properties(devkit_gfx PROPERTIES
    EXPORT_NAME gfx
    FOLDER "devkit/modules"
)

if(DEVKIT_BUILD_TESTS)
    add_executable(devkit_gfx_tests
        tests/gfx_test.cpp
    )
    target_link_libraries(devkit_gfx_tests PRIVATE
        devkit::gfx
        GTest::gtest
        GTest::gtest_main
    )
    set_target_properties(devkit_gfx_tests PROPERTIES FOLDER "devkit/modules/gfx/tests")
    gtest_discover_tests(devkit_gfx_tests)
endif()
