include("${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/system_link.cmake")

function(guess_what_setup_dependencies)
#    CPMAddPackage(
#            NAME TL_EXPECTED
#            GITHUB_REPOSITORY TartanLlama/expected
#            VERSION 1.1.0
#            OPTIONS
#            "EXPECTED_BUILD_PACKAGE OFF"
#            "EXPECTED_BUILD_TESTS OFF"
#            "EXPECTED_BUILD_PACKAGE_DEB OFF"
#            "BUILD_SHARED_LIBS OFF"
#    )
    CPMAddPackage(
            NAME TL_OPTIONAL
            GITHUB_REPOSITORY TartanLlama/optional
            VERSION 1.1.0
            OPTIONS
            "OPTIONAL_BUILD_PACKAGE OFF"
            "OPTIONAL_BUILD_TESTS OFF"
            "OPTIONAL_BUILD_PACKAGE_DEB OFF"
            "BUILD_SHARED_LIBS OFF"
    )
    CPMAddPackage(
            NAME LIB2K
            GITHUB_REPOSITORY mgerhold/lib2k
            VERSION 0.1.2
            OPTIONS
            "BUILD_SHARED_LIBS OFF"
    )
#    CPMAddPackage(
#            NAME MAGIC_ENUM
#            GITHUB_REPOSITORY Neargye/magic_enum
#            VERSION 0.9.6
#    )
endfunction()
