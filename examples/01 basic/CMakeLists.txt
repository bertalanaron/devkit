# List sources
set(SOURCES
    src/main.cpp)
source_group("src" FILES ${SOURCES})

# Create executable
add_executable(01_basic ${SOURCES})
target_link_libraries(01_basic PRIVATE devkit)
set_target_properties(01_basic PROPERTIES FOLDER "examples")

# Include common shaders
include(${CMAKE_SOURCE_DIR}/examples/_common_data/shaders.cmake)
target_sources(01_basic PRIVATE ${SHADER_FILES})

set(DATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/examples/_common_data/")
set(DATA_LINK_NAME "${CMAKE_CURRENT_BINARY_DIR}/data")
add_custom_command(
    OUTPUT "${SHADER_LINK_NAME}"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${SHADER_SOURCE_DIR}" "${SHADER_LINK_NAME}"
    COMMENT "Creating symbolic link to shaders directory"
)
add_custom_target(link_shaders ALL DEPENDS "${SHADER_LINK_NAME}")
add_dependencies(01_basic link_shaders)


