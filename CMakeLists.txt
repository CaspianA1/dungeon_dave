cmake_minimum_required(VERSION 3.0)

project(DungeonDave)

########## Exporting CMake's compile commands, so that Vim's YouCompleteMe can find any project headers.

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# On the first build, this file will not exist yet, so this avoids a failing shell command.
# This moves the file one directory outwards, so that YouCompleteMe can find it.
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
	execute_process(COMMAND sh -c "mv compile_commands.json ..")
endif()

########## Setting the C standard, warning flags, and debug/release flags.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CONVERSION_WARNINGS_1 "-Wfloat-conversion -Wdouble-promotion -Wsign-conversion")
set(CONVERSION_WARNINGS_2 "-Wimplicit-int-conversion -Wshorten-64-to-32 -Wincompatible-pointer-types")
set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic -Wformat ${CONVERSION_WARNINGS_1} ${CONVERSION_WARNINGS_2}")

set(CMAKE_C_FLAGS_DEBUG "-g -fsanitize=address,undefined,float-divide-by-zero,unsigned-integer-overflow,implicit-conversion")
set(CMAKE_C_FLAGS_RELEASE "-Ofast")

########## Declaring SDL2 as a package, and other standard CMake boilerplate.

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB_RECURSE SRC_FILES src/*.c)

add_executable(dungeon_dave ${SRC_FILES})
target_include_directories(dungeon_dave PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dungeon_dave ${SDL2_LIBRARIES})

##########
