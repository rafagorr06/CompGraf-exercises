cmake_minimum_required(VERSION 3.10)
project(shadowm)

set(CMAKE_CXX_STANDARD 17)
file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB UTILS_SOURCES common/utils/*.cpp)
file(GLOB IMGUI_SOURCES common/third/imgui/*.cpp)

add_compile_definitions(GLFW_INCLUDE_NONE)

set(SOURCES
  ${SOURCE_FILES}
  ${UTILS_SOURCES}
  ${IMGUI_SOURCES}
  common/third/glad/glad.c
  common/third/stb/stb_image.c
  common/third/imgui/backends/imgui_impl_glfw.cpp
  common/third/imgui/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(shadowm ${SOURCES})

# Add include directories
target_include_directories(shadowm PRIVATE
    common/third/glad
    common/third/imgui
    common/third/stb
    common/utils
)

# Link system libraries
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
target_link_libraries(shadowm PRIVATE glfw)
target_link_libraries(shadowm PRIVATE glm::glm)

# Copy assets folder to the target's output directory after building
add_custom_command(TARGET shadowm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/shaders"
        "$<TARGET_FILE_DIR:shadowm>/shaders"
)
add_custom_command(TARGET shadowm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/models"
        "$<TARGET_FILE_DIR:shadowm>/models"
)
