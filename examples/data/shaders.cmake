# if(NOT SHADER_FILES)

# Add shaders to the project
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/examples/_common_data/shaders/*.*")
source_group("shaders" FILES ${SHADER_FILES})

# endif()
