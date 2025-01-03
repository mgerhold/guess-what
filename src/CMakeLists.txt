add_executable(main
        main.cpp
        parser.hpp
        parser.cpp
        command.hpp
        room.hpp
        file_parser.hpp
        file_parser.cpp
)

target_link_system_libraries(main
        PRIVATE
        lib2k
)
