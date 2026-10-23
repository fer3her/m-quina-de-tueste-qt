cmake_minimum_required(VERSION 3.16)
project(RoastMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

add_executable(roastmonitor src/main.cpp)
target_link_libraries(roastmonitor PRIVATE Qt6::Widgets Qt6::Charts)