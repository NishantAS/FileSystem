cmake_minimum_required(VERSION 3.21)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

project(FileSystem VERSION 1.0 LANGUAGES CXX)

file(GLOB SOURCES "src/*.cpp")
file(GLOB PRIVATE_HEADERS "src/*.hpp")
file(GLOB PUBLIC_HEADERS "include/*.hpp")

add_executable(FileSystem ${SOURCES} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})
target_include_directories(FileSystem PUBLIC include)

install(TARGETS FileSystem DESTINATION bin)