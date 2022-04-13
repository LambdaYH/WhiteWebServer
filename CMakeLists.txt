cmake_minimum_required(VERSION 3.1)
project(WhiteWebServer)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED) # pthread

set(JSONCPP_WITH_TESTS OFF)
add_subdirectory(third-party/jsoncpp)

include_directories(
    Sources
    third-party/jsoncpp/include
)

aux_source_directory(Sources/logger LOGGER_SRC)
aux_source_directory(Sources/pool POOL_SRC)
aux_source_directory(Sources/protocol/http PROTOCOL_HTTP_SRC)
aux_source_directory(Sources/timer TIMER_SRC)
aux_source_directory(Sources/server SERVER_SRC)
aux_source_directory(Sources/epoll EPOLL_SRC)
aux_source_directory(Sources/buffer BUFFER_SRC)
aux_source_directory(Sources/config CONFIG_SRC)

add_executable(${PROJECT_NAME} 
                Sources/main.cpp 
                ${POOL_SRC} 
                ${PROTOCOL_HTTP_SRC} 
                ${LOGGER_SRC}
                ${TIMER_SRC}
                ${SERVER_SRC}
                ${EPOLL_SRC}
                ${BUFFER_SRC}
                ${CONFIG_SRC}
                )
                
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads jsoncpp_lib)