cmake_minimum_required(VERSION 3.22)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(XadrezStockfish LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Pasta do executavel e recursos")
foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${configuration} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endforeach()
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
# GLFW and GLAD ship as portable sources. No installed GLFW/GLEW is used.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Wayland backend (optional on Linux)")
set(BUILD_SHARED_LIBS OFF)
find_package(assimp 5 CONFIG QUIET)
if(NOT TARGET assimp::assimp)
    include(FetchContent)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    FetchContent_Declare(assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.tar.gz
        URL_HASH SHA256=66dfbaee288f2bc43172440a55d0235dfc7bf885dda6435c038e8000e79582cb)
    FetchContent_MakeAvailable(assimp)
endif()
add_subdirectory(vendor/glfw EXCLUDE_FROM_ALL)
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)
target_link_libraries(imgui PUBLIC glfw ${CMAKE_DL_LIBS})
if(MSVC)
    target_compile_options(imgui PRIVATE /utf-8)
endif()
set(STOCKFISH_LOCAL "" CACHE FILEPATH "Stockfish alternativo (opcional)")
set(engine_arguments --output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(STOCKFISH_LOCAL)
    list(APPEND engine_arguments --engine "${STOCKFISH_LOCAL}")
endif()
add_custom_target(prepare_stockfish
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/setup_engine.py" ${engine_arguments}
    COMMENT "Preparando Stockfish local" VERBATIM)
file(GLOB graphics_sources CONFIGURE_DEPENDS src/graphics/*.cpp src/pieces/*.cpp)
add_executable(xadrez src/main.cpp src/jogo.cpp src/stockfish.cpp ${graphics_sources})
add_custom_target(prepare_shaders
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMENT "Copiando shaders GLSL" VERBATIM)
add_dependencies(xadrez prepare_shaders)
add_custom_target(prepare_assets
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMENT "Copiando assets dos cenários" VERBATIM)
add_dependencies(xadrez prepare_assets)
target_include_directories(xadrez PRIVATE include src vendor/chess-library vendor/glfw/deps)
target_link_libraries(xadrez PRIVATE imgui Threads::Threads assimp::assimp)
if(WIN32)
    # Installed Assimp packages may provide a DLL instead of a static library.
    get_target_property(assimp_type assimp::assimp TYPE)
    if(assimp_type STREQUAL "SHARED_LIBRARY")
        add_custom_command(TARGET xadrez POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "$<TARGET_FILE:assimp::assimp>" "$<TARGET_FILE_DIR:xadrez>" VERBATIM)
    endif()
endif()
if(MINGW)
    target_link_libraries(xadrez PRIVATE gdi32)
    target_link_options(xadrez PRIVATE -static)
endif()
if(MSVC)
    target_compile_options(xadrez PRIVATE /W4 /utf-8)
else()
    target_compile_options(xadrez PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_dependencies(xadrez prepare_stockfish)
