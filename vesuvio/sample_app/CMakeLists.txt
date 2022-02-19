cmake_minimum_required(VERSION 3.0)
project(sample_app)

message(STATUS "project=${CMAKE_PROJECT_NAME}")
message(STATUS "runtime_DEFINITIONS=${runtime_DEFINITIONS}")
message(STATUS "runtime_INCLUDE_DIRS=${runtime_INCLUDE_DIRS}")

file(GLOB CPP_FILES *.cpp)

set(CMAKE_CXX_STANDARD 17)
#add_compile_options(-Wall -Wextra)
add_definitions(${runtime_DEFINITIONS})
add_executable(${CMAKE_PROJECT_NAME} ${CPP_FILES})

include_directories(
    ${runtime_INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    runtime
)