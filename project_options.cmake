include(${PROJECT_SOURCE_DIR}/cmake/warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sanitizers.cmake)

# the following function was taken from:
# https://github.com/cpp-best-practices/cmake_template/blob/main/ProjectOptions.cmake
macro(check_sanitizer_support)
    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
        set(supports_ubsan ON)
    else ()
        set(supports_ubsan OFF)
    endif ()

    if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
        set(supports_asan OFF)
    else ()
        set(supports_asan ON)
    endif ()
endmacro()

if (PROJECT_IS_TOP_LEVEL)
    option(guess_what_warnings_as_errors "Treat warnings as errors" ON)
    option(guess_what_enable_undefined_behavior_sanitizer "Enable undefined behavior sanitizer" ${supports_ubsan})
    option(guess_what_enable_address_sanitizer "Enable address sanitizer" ${supports_asan})
    option(guess_what_build_tests "Build unit tests" ON)
else ()
    option(guess_what_warnings_as_errors "Treat warnings as errors" OFF)
    option(guess_what_enable_undefined_behavior_sanitizer "Enable undefined behavior sanitizer" OFF)
    option(guess_what_enable_address_sanitizer "Enable address sanitizer" OFF)
    option(guess_what_build_tests "Build unit tests" OFF)
endif ()
option(guess_what_build_shared_libs "Build shared libraries instead of static libraries" ON)
set(BUILD_SHARED_LIBS ${guess_what_build_shared_libs})

add_library(guess_what_warnings INTERFACE)
guess_what_set_warnings(guess_what_warnings ${guess_what_warnings_as_errors})

add_library(guess_what_sanitizers INTERFACE)
guess_what_enable_sanitizers(guess_what_sanitizers ${guess_what_enable_address_sanitizer} ${guess_what_enable_undefined_behavior_sanitizer})

add_library(guess_what_project_options INTERFACE)
target_link_libraries(guess_what_project_options
        INTERFACE guess_what_warnings
        INTERFACE guess_what_sanitizers
)
