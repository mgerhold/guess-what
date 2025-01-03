add_executable(main
        main.cpp
        parser.hpp
        parser.cpp
        command.hpp
)

target_link_system_libraries(main
        PRIVATE
        lib2k
)
