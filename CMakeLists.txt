# example:
#   cd gnome-arcade/build
#
#	cmake ..
#	OR
# 	cmake -DAPP_RES=\"/usr/share/gnome-arcade\" -DCMAKE_C_FLAGS="-g -O2" ..
#	OR
# 	cmake -DMAME_BIN=\"/usr/games/mame\" -DCMAKE_C_FLAGS="-g -O2" ..
#	OR
#	cmake -DAPP_RES=\"/usr/share/gnome-arcade\" -DCMAKE_C_FLAGS="-g -O2 -fmessage-length=0 -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables" ..
#
#	make

# Set the name and the supported language of the project
project(gnome-arcade C)

# Set the minimum version of cmake required to build this project
cmake_minimum_required(VERSION 2.8)

#preprocesor flag
if (DEFINED APP_RES)
	add_definitions(-DAPP_RES=${APP_RES})
	MESSAGE(STATUS "APP_RES:" ${APP_RES})
else (NOT DEFINED ${APP_RES})
	add_definitions(-DAPP_RES=".")
	message(STATUS "APP_RES is NOT DEFINED! Assuming '.' (-DAPP_RES=\".\")")
endif (DEFINED APP_RES)

if (DEFINED MAME_BIN)
	add_definitions(-DMAME_BIN=${MAME_BIN})
	MESSAGE(STATUS "MAME_BIN:" ${MAME_BIN})
else (NOT DEFINED ${MAME_BIN})
	add_definitions(-DMAME_BIN="/usr/bin/mame")
	message(STATUS "MAME_BIN is NOT DEFINED! Assuming '/usr/bin/mame' (-DMAME_BIN=\"/usr/bin/mame\")")
endif (DEFINED MAME_BIN)

# the compiler flags for compiling C sources
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wpedantic -Wformat -Wformat-security -Wunused-result -march=native")

# log
MESSAGE(STATUS "CMAKE_C_FLAGS:" ${CMAKE_C_FLAGS})

# GTK
# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0>=3.10)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# Add src
add_subdirectory(src)

