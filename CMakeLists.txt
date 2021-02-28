cmake_minimum_required(VERSION 3.0.0)
project(MazeGame VERSION 0.1.0)
set( CMAKE_CXX_STANDARD 17 )

add_executable(MazeGame Src/Maze.cc Src/TerrainGenerator.cc)

add_subdirectory(Flux/FluxEngine)
target_link_libraries(MazeGame PUBLIC FluxEngine)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --preload-file ../Assets --preload-file ../shaders")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_WEBGL2=1 -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s LLD_REPORT_UNDEFINED")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=1 -g4 --source-map-base \"http://0.0.0.0:8000/\" -s DISABLE_EXCEPTION_CATCHING=2") # Debug 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math") # Release

    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -lglfw -s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=1 -g4")
endif()
