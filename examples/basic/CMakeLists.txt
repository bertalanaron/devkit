add_executable(examples_basic examples_basic.cpp)
target_link_libraries(examples_basic PRIVATE ${PROJECT_NAME})

#add_custom_command(
#    TARGET examples_basic
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${CMAKE_SOURCE_DIR}/examples/basic/out"
#            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/out"
#)

set_target_properties(examples_basic PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/basic/out"
)