cmake_minimum_required(VERSION 3.10)
project(min CXX)

# КРИТИЧНО для PSP: без этого newlib использует firmware 150 и syscall'ы не работают
add_compile_options(-D_PSP_FW_VERSION=600)
add_compile_options(-G0 -O2)

add_executable(min main.cpp)
target_link_libraries(min pspkernel)
