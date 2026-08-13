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
    include/devkit/algo/marching_squares.h
)

set(ALGO_PRIVATE_SOURCES
    src/algo/geometry.cpp
    src/algo/navmesh.cpp
    src/algo/navmesh_pathfinding.cpp
    src/algo/funnel.cpp
    src/algo/polypartition/polypartition.h
    src/algo/polypartition/polypartition.cpp
)

add_library(devkit_algo STATIC
    ${ALGO_PUBLIC_SOURCES}
    ${ALGO_PRIVATE_SOURCES}
)
add_library(devkit::algo ALIAS devkit_algo)

source_group("algo/include" FILES ${ALGO_PUBLIC_SOURCES})
source_group("algo/src" FILES ${ALGO_PRIVATE_SOURCES})

target_include_directories(devkit_algo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/common/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/gfx/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/io/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(devkit_algo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(devkit_algo PUBLIC
    devkit::common
    poly2tri::poly2tri
)

target_compile_options(devkit_algo PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

set_target_properties(devkit_algo PROPERTIES
    EXPORT_NAME algo
    FOLDER "devkit/modules"
)

if(DEVKIT_BUILD_TESTS)
    add_executable(devkit_algo_tests
        tests/algo_test.cpp
    )
    target_link_libraries(devkit_algo_tests PRIVATE
        devkit::algo
        GTest::gtest
        GTest::gtest_main
    )
    set_target_properties(devkit_algo_tests PROPERTIES FOLDER "devkit/modules/algo/tests")
    gtest_discover_tests(devkit_algo_tests)
endif()
