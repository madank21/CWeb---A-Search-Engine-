cmake_minimum_required(VERSION 3.10)
project(cweb_search_engine C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -std=c17)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB SOURCES
    "src/*.c"
)

add_executable(cweb_server ${SOURCES})

if(WIN32)
    target_link_libraries(cweb_server ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(cweb_server Threads::Threads)
endif()
