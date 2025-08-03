add_subdirectory("00 select")
add_subdirectory("01 basic")
add_subdirectory("02 rts")
add_subdirectory("03 terrain")

# Configure ini
get_filename_component(DATA_PATH "${CMAKE_SOURCE_DIR}/examples/data" ABSOLUTE)
configure_file(
    ${CMAKE_SOURCE_DIR}/examples/examples.ini.in
    ${CMAKE_BINARY_DIR}/examples.ini
    @ONLY
)
