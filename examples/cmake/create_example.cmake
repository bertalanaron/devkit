function(create_example example_name)
    # Get the rest of the arguments as sources
    set(example_sources ${ARGN})

    # Create executable
	add_executable(${example_name} ${example_sources})
    target_link_libraries(${example_name} PRIVATE devkit)
    set_target_properties(${example_name} PROPERTIES FOLDER "examples")
    
    # Move examples.ini next to exe at build
    add_custom_command(TARGET ${example_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/examples.ini"
            "$<TARGET_FILE_DIR:${example_name}>/examples.ini"
    )
endfunction()
