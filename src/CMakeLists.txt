# Add the necessary include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${SDL2_INCLUDE_DIRS})

# Files to be included
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/APU/*.cpp
    ${CMAKE_SOURCE_DIR}/src/APU/Channels/*.cpp
    ${CMAKE_SOURCE_DIR}/src/CPU/*.cpp
    ${CMAKE_SOURCE_DIR}/src/IO/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/enums/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/MBC/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC0/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC1/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC3/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC5/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Pipleline/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Utility/*.cpp
    
    # ImGui sources
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imconfig.h
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_internal.h
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_rectpack.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_textedit.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_truetype.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/backends/imgui_impl_sdlrenderer2.cpp
)

# Executable defention
add_executable(GameBoyEmulator ${SOURCES})

# Link SDL2
target_link_libraries(GameBoyEmulator SDL2)

# Add source files of ImGui
target_include_directories(GameBoyEmulator PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/backends
)

# Ad source files directories
target_include_directories(GameBoyEmulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/APU
    ${CMAKE_SOURCE_DIR}/src/APU/Channels
    ${CMAKE_SOURCE_DIR}/src/CPU
    ${CMAKE_SOURCE_DIR}/src/IO
    ${CMAKE_SOURCE_DIR}/src/Memory/enums
    ${CMAKE_SOURCE_DIR}/src/Memory/MBC
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC0
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC1
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC3
    ${CMAKE_SOURCE_DIR}/src/Memory/MBCS/MBC5
    ${CMAKE_SOURCE_DIR}/src/Pipleline
    ${CMAKE_SOURCE_DIR}/src/Utility
)

