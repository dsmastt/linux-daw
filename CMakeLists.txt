cmake_minimum_required(VERSION 3.12)

project(linux_daw LANGUAGES CXX)

add_executable(linux-daw src/main.cpp)
target_compile_features(linux-daw PRIVATE cxx_std_20)
