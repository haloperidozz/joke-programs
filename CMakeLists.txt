cmake_minimum_required(VERSION 3.10)
project(JokePrograms VERSION 1.0)

if (NOT WIN32)
    message(FATAL_ERROR "This project can only be built on MS Windows :(")
endif ()

set(CMAKE_EXECUTABLE_SUFFIX ".EXE")

add_compile_definitions(_UNICODE UNICODE)

if (MINGW)
    add_compile_options(-municode)
    add_link_options(-municode -static-libgcc -static-libstdc++ -static)
endif ()

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_link_options(/MANIFEST:NO)
endif ()

add_subdirectory(ALERTA.EXE_SRC)
add_subdirectory(MOUSEMVD.EXE_SRC)
add_subdirectory(RECYCLE.EXE_SRC)