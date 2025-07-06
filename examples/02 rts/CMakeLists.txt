# List sources
set(SOURCES
    src/main.cpp)
source_group("src" FILES ${SOURCES})

# Create executable
add_executable(02_rts ${SOURCES})
target_link_libraries(02_rts PRIVATE devkit)
set_target_properties(02_rts PROPERTIES FOLDER "examples")


