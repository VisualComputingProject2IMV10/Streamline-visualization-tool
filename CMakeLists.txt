cmake_minimum_required(VERSION 3.30)
project(VCP)

set(CMAKE_CXX_STANDARD 14)

include_directories(/)

# Add ImGui include directory
include_directories(imgui)
include_directories(imgui/backends)

# Copy shader files to build directory
file(GLOB SHADER_FILES
        "${CMAKE_SOURCE_DIR}/src/*.vs"
        "${CMAKE_SOURCE_DIR}/src/*.fs"
)
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../)

add_executable(VCP
        core/DataReader.cpp
        extra/glad.c
        ../Source.cpp
        core/VectorField.cpp
        core/StreamlineTracer.cpp
        core/StreamlineRenderer.cpp

        # ImGui core files
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp


        # ImGui backend files
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)

#find_package(Eigen3 REQUIRED)
#include_directories(${EIGEN3_INCLUDE_DIRS})



find_package(GLFW3 REQUIRED)
if(GLFW3_FOUND)
    target_include_directories(VCP PRIVATE ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(VCP PRIVATE glfw)
endif()

target_include_directories(VCP PRIVATE /opt/homebrew/include)
target_link_libraries(VCP PRIVATE /opt/homebrew/lib/libglfw.3.4.dylib)

#target_link_libraries(VCP
#        ${EIGEN3_LIBRARIES}
#)

if(APPLE)
    target_link_libraries(VCP PRIVATE
            "-framework Cocoa"
            "-framework OpenGL"
            "-framework IOKit"
            "-framework CoreVideo")
endif()

find_package(glm REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)