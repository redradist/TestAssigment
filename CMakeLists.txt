cmake_minimum_required(VERSION 3.12)
project(untitled1)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_CLANG_TIDY "CMAKE_CXX_CLANG_TIDY-NOTFOUND")
if (CMAKE_CXX_CLANG_TIDY)
    message(STATUS "CMAKE_CXX_CLANG_TIDY WHAT ???")
else()
    set(CMAKE_CXX_CLANG_TIDY "")
endif()

add_executable(untitled1 main.cpp)
target_link_libraries(untitled1 pthread boost_program_options)