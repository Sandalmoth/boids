cmake_minimum_required(VERSION 3.0.0)

add_executable(boids main.cpp)

# Libraries
find_package (SDL2)
if (SDL2_FOUND)
  include_directories(${SDL2_INCLUDE_DIRS})
  target_link_libraries (boids ${SDL2_LIBRARIES} -lSDL2)
endif (SDL2_FOUND)


# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -ggdb") # debug compilation
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -Werror")
endif()
