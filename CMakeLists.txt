cmake_minimum_required(VERSION 3.23)

project (superbible)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(MY_DEBUG_OPTIONS /Zi /DDEBUG /D_DEBUG /D_CONSOLE /MDd)
set(MY_RELEASE_OPTIONS /O2 /MD)

add_compile_options(
    /W3 /EHsc /FC /nologo 
    "$<$<CONFIG:Debug>:${MY_DEBUG_OPTIONS}>"
    "$<$<CONFIG:Release>:${MY_RELEASE_OPTIONS}>"
)

set(MY_DEBUG_LINKER_OPTIONS glfw3.lib glew32.lib opengl32.lib irrKlang.lib)
set(MY_RELEASE_LINKER_OPTIONS Effects11.lib imgui.obj DirectXTK.lib)

add_link_options(
    /LIBPATH:..\\ThirdParty\\irrKlang\\lib\\Winx64-visualStudio
    /LIBPATH:..\\ThirdParty\\glew-2.1.0\\lib\\Release\\x64
    /LIBPATH:..\\ThirdParty\\glfw\\lib-vc2022
    "$<$<CONFIG:Debug>:${MY_DEBUG_LINKER_OPTIONS}>"
    "$<$<CONFIG:Release>:${MY_RELEASE_LINKER_OPTIONS}>"
)

include_directories(ThirdParty\\glew-2.1.0\\include)
include_directories(ThirdParty\\glm)
include_directories(headers)
include_directories(ThirdParty\\imgui)
include_directories(ThirdParty\\irrKlang\\include)
include_directories(ThirdParty\\OBJ-loader)
include_directories(ThirdParty\\glfw\\include)
include_directories(ThirdParty\\tinygltf)

file(GLOB book_sources bluebook/**/*.cpp)

set(SOURCE
    source/GL_Helpers.cpp
    source/Mesh.cpp
    source/System.cpp
    source/Texture.cpp
    source/boilerplate_main.cpp
    ${book_sources}
    ThirdParty/imgui/imgui.cpp
    ThirdParty/imgui/imgui_demo.cpp
    ThirdParty/imgui/imgui_draw.cpp
    ThirdParty/imgui/imgui_impl_glfw.cpp
    ThirdParty/imgui/imgui_impl_opengl3.cpp
    ThirdParty/imgui/imgui_tables.cpp
    ThirdParty/imgui/imgui_widgets.cpp
)

add_executable (${PROJECT_NAME} ${SOURCE})
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")