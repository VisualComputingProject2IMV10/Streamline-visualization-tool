cmake_minimum_required(VERSION 3.30)
project(VCP)

set(CMAKE_CXX_STANDARD 14)

include_directories(/)

# Add ImGui include directory
include_directories(imgui)
include_directories(imgui/backends)

add_executable(VCP
        streamline-visualization/src/core/DataReader.cpp
        streamline-visualization/src/extra/glad.c
        streamline-visualization/src/Source.cpp
        streamline-visualization/src/core/VectorField.cpp
        streamline-visualization/src/core/StreamlineTracer.cpp
        streamline-visualization/src/core/StreamlineRenderer.cpp
        streamline-visualization/src/core/MouseSampler.cpp

        # ImGui core files
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp


        # ImGui backend files
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
 )

add_custom_command(
    TARGET VCP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory              
        ${CMAKE_SOURCE_DIR}/streamline-visualization/src/shaders $<TARGET_FILE_DIR:VCP>/shaders
    COMMENT "Copying shaders" VERBATIM
)

add_custom_command(
    TARGET VCP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory              
        ${CMAKE_SOURCE_DIR}/streamline-visualization/src/data $<TARGET_FILE_DIR:VCP>/data
    COMMENT "Copying nifti files" VERBATIM
)

find_package(GLFW3 REQUIRED)

target_link_libraries(VCP PRIVATE glfw)

if(APPLE)
    target_include_directories(VCP PRIVATE /opt/homebrew/include)
    target_link_libraries(VCP PRIVATE /opt/homebrew/lib/libglfw.3.4.dylib)

    target_link_libraries(VCP PRIVATE
            "-framework Cocoa"
            "-framework OpenGL"
            "-framework IOKit"
            "-framework CoreVideo")
endif()

find_package(glm CONFIG  REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)