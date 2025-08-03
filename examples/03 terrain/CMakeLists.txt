include(../cmake/create_example.cmake)

set(SOURCES
    src/main.cpp
    src/terrain.h
    src/terrain_editor.h
    src/terrain.cpp
    src/terrain_editor.cpp)
source_group("src" FILES ${SOURCES})
	
create_example(03_terrain ${SOURCES})
