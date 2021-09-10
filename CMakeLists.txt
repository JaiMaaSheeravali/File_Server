# only for cmake --version >= 3.5.1
cmake_minimum_required(VERSION 3.5.1)

# project name
project(server)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
# creates the variable EXEC and sets it to hello
set(EXEC server)

# set the C++17 standard
set(CMAKE_CXX_STANDARD 17)

# I../includes
include_directories(include)

# puts all .cpp files inside src to the SOURCES variable
file(GLOB SOURCES src/*.c*)

# compiles the files defined by SOURCES to generates the executable defined by EXEC
add_executable(${EXEC} ${SOURCES})

# include pthread library
find_package (Threads REQUIRED)

target_link_libraries(${EXEC}
    ${CMAKE_THREAD_LIBS_INIT}
)