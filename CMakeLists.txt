cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

project(demod)

set(CMAKE_C_FLAGS_RELEASE "-Ofast -march=nehalem -mtune=nehalem")


add_executable(demod
	demod.c
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# liquid-dsp
find_package(Liquid REQUIRED)
include_directories(${LIQUID_INCLUDE_DIR})
target_link_libraries(demod ${LIQUID_LIBRARIES})

# sys math lib
target_link_libraries(demod m)

install(TARGETS demod DESTINATION bin)
