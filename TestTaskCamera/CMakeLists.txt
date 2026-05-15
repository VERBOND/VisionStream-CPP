cmake_minimum_required(VERSION 4.2)
project(TestTaskCamera)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

add_executable(TestTaskCamera main.cpp)

target_link_libraries(TestTaskCamera PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
)