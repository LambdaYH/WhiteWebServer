cmake_minimum_required(VERSION 3.1)
project(WhiteWebServer)
set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)

include_directories(Sources)

aux_source_directory(Sources/logger LOGGER_SRC)
aux_source_directory(Sources/pool POOL_SRC)
aux_source_directory(Sources/protocol/http PROTOCOL_HTTP_SRC)
aux_source_directory(Sources/timer TIMER_SRC)
aux_source_directory(Sources/server SERVER_SRC)
aux_source_directory(Sources/epoll EPOLL_SRC)
aux_source_directory(Sources/buffer BUFFER_SRC)

add_executable(${PROJECT_NAME} 
                Sources/main.cpp 
                ${POOL_SRC} 
                ${PROTOCOL_HTTP_SRC} 
                ${LOGGER_SRC}
                ${TIMER_SRC}
                ${SERVER_SRC}
                ${EPOLL_SRC}
                ${BUFFER_SRC})
                
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)