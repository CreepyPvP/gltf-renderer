cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(strategy)
# Clang deez nuts
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include debug information
set(CMAKE_BUILD_TYPE Debug)

# Variables for Paths of External Libraries
set(GLFW_ROOT_DIR external/glfw)
set(GLM_ROOT_DIR external/glm)
set(IMGUI_ROOT_DIR external/imgui)
set(FASTGLTF_ROOT_DIR external/fastgltf)
set(GLAD_ROOT_DIR external/glad)
set(SIMDJSON_ROOT_DIR external/simdjson)

# Add all Header Files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/include/*.h
	${CMAKE_SOURCE_DIR}/include/*.hpp
)


IF (WIN32)
    # Windows libraries:
    set(LIBS glfw opengl32 glm glad)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/windows.cpp

        # IMGUI
        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp

        ${FASTGLTF_ROOT_DIR}/src/*.cpp
        
        ${SIMDJSON_ROOT_DIR}/simdjson.cpp
    )

ELSE()
    # Linux libraries:
    set(LIBS glfw X11 glm glad)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/linux.cpp

        # IMGUI
        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp

        ${FASTGLTF_ROOT_DIR}/src/*.cpp
    )
ENDIF()

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Add the External Libraries / Files Directory
add_subdirectory(${GLFW_ROOT_DIR})
add_subdirectory(${GLM_ROOT_DIR})
add_subdirectory(${GLAD_ROOT_DIR})

target_include_directories(${PROJECT_NAME} 
	PUBLIC ${GLFW_ROOT_DIR}/include
    PUBLIC ${IMGUI_ROOT_DIR}
    PUBLIC ${IMGUI_ROOT_DIR}/backends
    PUBLIC ${GLM_ROOT_DIR}/include
    PUBLIC ${FASTGLTF_ROOT_DIR}/include
    PUBLIC ${GLAD_ROOT_DIR}/include/glad
    PUBLIC ${SIMDJSON_ROOT_DIR}
    PUBLIC .
)

# if(MSVC)
#     target_compile_options(${PROJECT_NAME} PUBLIC /W4)
# else()
#     target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)
# endif()

target_link_directories(${PROJECT_NAME} 
	PRIVATE ${GLFW_ROOT_DIR}/src
    PRIVATE ${GLM_ROOT_DIR}/src
    PRIVATE ${GLAD_ROOT_DIR}/src
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

