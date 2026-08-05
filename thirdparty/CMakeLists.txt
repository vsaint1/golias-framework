cmake_minimum_required(VERSION 3.18)

project(thirdparty)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_ENABLE_CXX_20 ON)
set(GLM_TEST_ENABLE OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build STATIC libs" FORCE)

add_subdirectory(glad)
add_subdirectory(stb_image)
add_subdirectory(stb_truetype)

add_subdirectory(glfw EXCLUDE_FROM_ALL)
add_subdirectory(glm)
add_subdirectory(spdlog)

add_subdirectory(json)
add_subdirectory(tinygltf)
add_subdirectory(tinyobj)


add_subdirectory(mini_audio)


add_library(thirdparty INTERFACE)

target_include_directories(${PROJECT_NAME}  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# TODO: later we check platform and add vulkan only for supported platforms
find_package(Vulkan REQUIRED)

message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")


target_link_libraries(${PROJECT_NAME}  INTERFACE
        # SDL3::SDL3
        glfw
        stb_image
        stb_truetype
        glm
        glad
        spdlog
        json
        tinygltf
        tinyobjloader
        mini_audio
        Vulkan::Vulkan
)