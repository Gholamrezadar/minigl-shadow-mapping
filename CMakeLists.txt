cmake_minimum_required(VERSION 4.0.0)
project(minigl_hello_triangle)
set(CMAKE_CXX_STANDARD 20)

set(TARGET_NAME "minigl_hello_triangle")

# build GLFW (static by default)
add_subdirectory(vendor/glfw)

# build glad (static)
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include/)

# build imgui
add_library(imgui
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
    vendor/glfw/include
)
target_link_libraries(imgui PUBLIC glfw)

# our target app
add_executable(${TARGET_NAME} main.cpp)

# our target include directories
target_include_directories(${TARGET_NAME} PRIVATE 
    vendor/glfw/include
    vendor/glad/include
    vendor/imgui
    vendor/imgui/backends
    vendor/glm
) 

# our target link libraries
target_link_libraries(${TARGET_NAME}
    glfw
    glad
    imgui
)

# Copy shaders to build directory
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${TARGET_NAME}>/shaders
)

# Copy resources to build directory
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
)