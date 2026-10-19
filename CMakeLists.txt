cmake_minimum_required(VERSION 3.16)
project(XadrezStockfish LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${configuration} "${CMAKE_BINARY_DIR}/bin")
endforeach()

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
set(STOCKFISH_LOCAL "" CACHE FILEPATH "Stockfish alternativo (opcional)")
set(engine_arguments --output "${CMAKE_BINARY_DIR}/bin")
if(STOCKFISH_LOCAL)
    list(APPEND engine_arguments --engine "${STOCKFISH_LOCAL}")
endif()
add_custom_target(prepare_stockfish
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/setup_engine.py" ${engine_arguments}
    COMMENT "Preparando Stockfish local para a plataforma"
    VERBATIM)

add_executable(xadrez src/main.cpp src/jogo.cpp src/stockfish.cpp src/autoteste.cpp)
target_include_directories(xadrez PRIVATE src vendor/chess-library)
target_link_libraries(xadrez PRIVATE Threads::Threads)
if(MINGW)
    target_link_options(xadrez PRIVATE -static)
endif()
if(MSVC)
    target_compile_options(xadrez PRIVATE /W4 /utf-8)
else()
    target_compile_options(xadrez PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_dependencies(xadrez prepare_stockfish)

enable_testing()
add_test(NAME stockfish_api COMMAND xadrez --self-test --time 50)
set_tests_properties(stockfish_api PROPERTIES TIMEOUT 90)
