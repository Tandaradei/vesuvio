cmake_minimum_required(VERSION 3.0)
project(gfx)

message(STATUS "project=${CMAKE_PROJECT_NAME}")

file(GLOB CPP_FILES *.cpp)

set(CMAKE_CXX_STANDARD 17)
#add_compile_options(-Wall -Wextra)
set(DEFINITIONS -DVSV_ENABLE_VULKAN)
add_definitions(${DEFINITIONS})

add_library(${CMAKE_PROJECT_NAME} STATIC ${CPP_FILES})

find_package(Vulkan REQUIRED)
message(STATUS "${Vulkan_LIBRARIES}")
set(GLFW_DIR /home/laurin/libs/glfw-3.3.3)

add_subdirectory(${GLFW_DIR} build)

message(STATUS "${GLFW_LIBRARIES}")

set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_DIR}/include
    ~/libs/VulkanMemoryAllocator/src
    ~/libs/stb
    ~/libs/glm
)

include_directories(${INCLUDE_DIRS})

target_link_libraries (${PROJECT_NAME} 
    ${Vulkan_LIBRARIES}
    glfw ${GLFW_LIBRARIES}
)

set(${PROJECT_NAME}_INCLUDE_DIRS ${INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_DEFINITIONS ${DEFINITIONS}
    CACHE INTERNAL "${PROJECT_NAME}: DEFINITIONS" FORCE)
