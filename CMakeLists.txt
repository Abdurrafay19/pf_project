cmake_minimum_required(VERSION 3.10)
project(sfml_project)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(sfml_project main.cpp)
target_link_libraries(sfml_project sfml-graphics sfml-window sfml-system)
