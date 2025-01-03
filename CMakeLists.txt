cmake_minimum_required(VERSION 3.27)
project(guess_what)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(dependencies.cmake)
guess_what_setup_dependencies()

include(project_options.cmake)

add_subdirectory(src bin)
