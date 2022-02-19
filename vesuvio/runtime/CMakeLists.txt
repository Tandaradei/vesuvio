cmake_minimum_required(VERSION 3.0)
project(runtime)

message(STATUS "project=${CMAKE_PROJECT_NAME}")
message(STATUS "gfx_DEFINITIONS=${gfx_DEFINITIONS}")
message(STATUS "gfx_INCLUDE_DIRS=${gfx_INCLUDE_DIRS}")

file(GLOB CPP_FILES *.cpp)

set(CMAKE_CXX_STANDARD 17)
#add_compile_options(-Wall -Wextra)
set(DEFINITIONS ${gfx_DEFINITIONS})
add_definitions(${DEFINITIONS})
add_library(${CMAKE_PROJECT_NAME} STATIC ${CPP_FILES})
set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}
    ${gfx_INCLUDE_DIRS}
)

include_directories(
    ${INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    gfx
)


set(${PROJECT_NAME}_INCLUDE_DIRS ${INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_DEFINITIONS ${DEFINITIONS}
    CACHE INTERNAL "${PROJECT_NAME}: DEFINITIONS" FORCE)
