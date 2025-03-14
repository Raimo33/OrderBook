cmake_minimum_required(VERSION 3.5)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(OrderBook
  main.cpp
  Client.cpp
  OrderBook.cpp
  Config.cpp
  utils.cpp
)

target_compile_options(OrderBook PRIVATE -Wall -Wextra -Werror -pedantic)
target_compile_options(OrderBook PRIVATE -O3 -march=native -flto)