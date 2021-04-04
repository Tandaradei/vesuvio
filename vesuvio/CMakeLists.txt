cmake_minimum_required(VERSION 3.0)

project(vesuvio)

set(vesuvio_VERSION_MAJOR 0)
set(vesuvio_VERSION_MINOR 1)

SET(BASEPATH "${CMAKE_SOURCE_DIR}/src")
SET(VULKAN_INCLUDE "/usr/include/vulkan")
SET(GLFW_INCLUDE "/home/laurin/libs/glfw-3.3.3/include")
SET(GLM_INCLUDE "/home/laurin/libs/glm")
SET(STB_INCLUDE "/home/laurin/libs/stb")

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} 
src/vesuvio.cpp
src/Application.hpp
src/Application.cpp
src/Vertex.hpp
src/UniformBufferObject.hpp
${VULKAN_INCLUDE}/vulkan.hpp
${GLFW_INCLUDE}/GLFW/glfw3.h
${GLM_INCLUDE}/glm/vec4.hpp
${GLM_INCLUDE}/glm/mat4x4.hpp
${GLM_INCLUDE}/glm/gtc/matrix_transform.hpp
${STB_INCLUDE}/stb_image.h
)

TARGET_INCLUDE_DIRECTORIES(vesuvio PRIVATE ${VULKAN_INCLUDE})
TARGET_INCLUDE_DIRECTORIES(vesuvio PRIVATE ${GLFW_INCLUDE})
TARGET_INCLUDE_DIRECTORIES(vesuvio PRIVATE ${GLM_INCLUDE})
TARGET_INCLUDE_DIRECTORIES(vesuvio PRIVATE ${STB_INCLUDE})

target_link_libraries(vesuvio glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

set_target_properties(vesuvio PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(vesuvio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "binaries/linux/${CMAKE_BUILD_TYPE}"
)
