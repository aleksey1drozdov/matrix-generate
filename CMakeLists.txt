cmake_minimum_required(VERSION 3.20)
project(MatrixAI CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(matrix_ai 
    main.cpp 
    core.h 
    transforms.h 
    dfs.h 
    symmetry_manager.h 
    file_handler.h
)

if(MINGW)
    target_link_options(matrix_ai PRIVATE -static-libgcc -static-libstdc++ -static -lpthread)
endif()

# Для тестирования (позже можно добавить Google Test или использовать простые ассерты)
# add_executable(tests tests.cpp core.h transforms.h)
